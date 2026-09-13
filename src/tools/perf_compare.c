/* Compare CPC compilation speed against tcc, and against an optional reference
   CPC build, over the portable C cases under Tests/benchmarks/compile.

   Every case is one translation unit.  A case carries metadata in the leading
   comment lines of its file:

     // PERF_NAME: c.some.name      display name (default: file name)
     // PERF_ARGS: -I... -D...      extra compiler arguments
     // PERF_SOURCE: src/foo.c      translation unit, relative to the tree root
     // PERF_TCC: no                skip tcc for this case (tcc cannot read it)
     // PERF_TIER: heavy            excluded from a -Fast run
     // PERF_ITERATIONS: 3          measured runs for this case

   Each compiler runs once per sample through CreateProcess, with its own output
   file and log, timed with QueryPerformanceCounter.  Compilations run strictly
   one at a time.  The median of the measured samples is reported; warmups are
   discarded.  The cpc-to-tcc and cpc-to-reference ratios are the portable
   numbers: tcc and the reference build absorb how fast today's machine is, so
   a ratio recorded on one day still means something on another.

   Usage: perf-compare [options]
     -Root DIR         tree root (default ".")
     -Cpc EXE          compiler under test (default ROOT/cpc.exe)
     -Tcc EXE          vendored tcc (default ROOT/third-party/tcc/win32/tcc.exe)
     -Reference EXE    reference CPC build, compared when given
     -Cases DIR        case directory (default ROOT/Tests/benchmarks/compile)
     -Out DIR          executables, logs and results (default ROOT/build/perf)
     -Results FILE     results TSV (default OUT/perf-results.tsv)
     -Baseline FILE    baseline TSV (default ROOT/Performance/baseline/perf-baseline.tsv)
     -Iterations N     measured runs per case (default 5; PERF_ITERATIONS wins)
     -Warmups N        discarded runs per case (default 1)
     -Tolerance PCT    allowed slowdown over the baseline (default 25)
     -UpdateBaseline   record this run as the baseline instead of gating it
     -NoGate           report only; never fail on a regression
     -Fast             skip heavy-tier cases
     -Log FILE         append one summary row to a shared performance log
     -Cycle N          cycle number recorded in -Log rows
     -Head REV         commit hash recorded in -Log rows
     -LastCycle        print the last cycle in -Log and exit
     -Quiet            suppress the per-case table
     -Help             print this text

   Exit: 0 ok, 1 compiler failure or speed regression, 2 usage or input error. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define MAX_CASES 64
#define MAX_SAMPLES 64
#define PATH_CAP 512
#define ARG_CAP 1024
#define CMD_CAP 8192

typedef struct {
  char name[96];
  char path[PATH_CAP];
  char source[PATH_CAP];
  char args[ARG_CAP];
  int use_tcc;
  int heavy;
  int iterations;
  int warmups;
} Case;

typedef struct {
  double cpc_ms;
  double tcc_ms;
  double ref_ms;
  int cpc_ok;
  int tcc_ok;
  int ref_ok;
} CaseResult;

typedef struct {
  char root[PATH_CAP];
  char cpc[PATH_CAP];
  char tcc[PATH_CAP];
  char ref[PATH_CAP];
  char cases[PATH_CAP];
  char out[PATH_CAP];
  char results[PATH_CAP];
  char raw_samples[PATH_CAP];
  char baseline[PATH_CAP];
  char log[PATH_CAP];
  char head[64];
  int iterations;
  int warmups;
  int cycle;
  double tolerance;
  int update_baseline;
  int no_gate;
  int fast;
  int quiet;
  int last_cycle;
  int history;
  int have_ref;
} Options;

static const char help_text[] =
  "usage: perf-compare [options]\n"
  "  -Root DIR        tree root (default \".\")\n"
  "  -Cpc EXE         compiler under test (default ROOT/cpc.exe)\n"
  "  -Tcc EXE         vendored tcc (default ROOT/third-party/tcc/win32/tcc.exe)\n"
  "  -Reference EXE   reference CPC build, compared when given\n"
  "  -Cases DIR       case directory (default ROOT/Tests/benchmarks/compile)\n"
  "  -Out DIR         executables, logs and results (default ROOT/build/perf)\n"
  "  -Results FILE    results TSV (default OUT/perf-results.tsv)\n"
  "  -RawSamples FILE write individual wall/CPU/load samples\n"
  "  -Baseline FILE   baseline TSV\n"
  "  -Iterations N    measured runs per case (default 5)\n"
  "  -Warmups N       discarded runs per case (default 1)\n"
  "  -Tolerance PCT   allowed slowdown over the baseline (default 25)\n"
  "  -UpdateBaseline  record this run as the baseline\n"
  "  -NoGate          report only; never fail on a regression\n"
  "  -Fast            skip heavy-tier cases\n"
  "  -Log FILE        append one summary row to a shared performance log\n"
  "  -Cycle N         cycle number recorded in -Log rows\n"
  "  -Head REV        commit hash recorded in -Log rows\n"
  "  -LastCycle       print the last cycle in -Log and exit\n"
  "  -History N       print the last N samples in -Log and exit (default 5)\n"
  "  -Quiet           suppress the per-case table\n";

static void die(const char *message)
{
  fprintf(stderr, "perf-compare: %s\n", message);
  exit(2);
}

static void copy_text(char *dst, size_t cap, const char *src)
{
  size_t i = 0;
  if (!cap) return;
  while (src && src[i] && i + 1 < cap) { dst[i] = src[i]; i++; }
  dst[i] = 0;
}

static int join_path(char *dst, size_t cap, const char *dir, const char *name)
{
  size_t n = strlen(dir);
  int needs_sep = n > 0 && dir[n - 1] != '\\' && dir[n - 1] != '/';
  return snprintf(dst, cap, "%s%s%s", dir, needs_sep ? "\\" : "", name) >= 0;
}

static void absolute_path(char *dst, size_t cap, const char *path)
{
  char buffer[PATH_CAP];
  DWORD n = GetFullPathNameA(path, (DWORD)sizeof buffer, buffer, 0);
  if (n == 0 || n >= sizeof buffer) copy_text(buffer, sizeof buffer, path);
  copy_text(dst, cap, buffer);
}

static int file_exists(const char *path)
{
  DWORD attrs = GetFileAttributesA(path);
  return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static int directory_exists(const char *path)
{
  DWORD attrs = GetFileAttributesA(path);
  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static int make_directory(const char *path)
{
  char work[PATH_CAP];
  char *cursor;
  if (directory_exists(path)) return 1;
  copy_text(work, sizeof work, path);
  for (cursor = work; *cursor; cursor++) {
    char saved;
    if (*cursor != '\\' && *cursor != '/') continue;
    /* Do not try to create the drive designator in an absolute path. */
    if (cursor == work + 2 && work[1] == ':') continue;
    saved = *cursor;
    *cursor = 0;
    if (work[0] && !directory_exists(work)) CreateDirectoryA(work, 0);
    *cursor = saved;
  }
  if (!directory_exists(work)) CreateDirectoryA(work, 0);
  return directory_exists(path);
}

static char *read_text(const char *path)
{
  FILE *file = fopen(path, "rb");
  long size;
  size_t got;
  char *text;
  if (!file) return 0;
  if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }
  size = ftell(file);
  if (size < 0 || size > 64L * 1024 * 1024) { fclose(file); return 0; }
  if (fseek(file, 0, SEEK_SET) != 0) { fclose(file); return 0; }
  text = (char *)malloc((size_t)size + 1);
  if (!text) { fclose(file); return 0; }
  got = fread(text, 1, (size_t)size, file);
  fclose(file);
  text[got] = 0;
  return text;
}

/* The raw file carries an inexpensive content identity for every executable
   and source used in a run.  It is intentionally labelled FNV-1a rather than
   presented as a cryptographic digest: it detects accidental workload/host
   changes while the recorded paths retain the exact audit trail. */
static unsigned long long file_hash64(const char *path)
{
  FILE *file = fopen(path, "rb");
  unsigned char bytes[4096];
  unsigned long long hash = 1469598103934665603ULL;
  size_t count;
  if (!file) return 0;
  while ((count = fread(bytes, 1, sizeof bytes, file)) != 0) {
    size_t i;
    for (i = 0; i < count; i++) {
      hash ^= bytes[i];
      hash *= 1099511628211ULL;
    }
  }
  fclose(file);
  return hash;
}

static void write_runtime_identity(FILE *raw, const Options *options)
{
  SYSTEM_INFO system;
  OSVERSIONINFOA version;
  char computer[256];
  DWORD computer_size = sizeof computer;
  memset(&version, 0, sizeof version);
  version.dwOSVersionInfoSize = sizeof version;
  GetNativeSystemInfo(&system);
  GetVersionExA(&version);
  if (!GetComputerNameA(computer, &computer_size)) copy_text(computer, sizeof computer, "unavailable");
  fprintf(raw, "# root\t%s\n", options->root);
  fprintf(raw, "# runtime\tcomputer=%s; os=%lu.%lu build=%lu; architecture=%u\n", computer,
          (unsigned long)version.dwMajorVersion, (unsigned long)version.dwMinorVersion,
          (unsigned long)version.dwBuildNumber, (unsigned)system.wProcessorArchitecture);
  fprintf(raw, "# compiler\tcpc\t%s\tfnv1a64=%016llx\n", options->cpc, file_hash64(options->cpc));
  if (file_exists(options->tcc))
    fprintf(raw, "# compiler\ttcc\t%s\tfnv1a64=%016llx\n", options->tcc, file_hash64(options->tcc));
  if (options->have_ref)
    fprintf(raw, "# compiler\treference\t%s\tfnv1a64=%016llx\n", options->ref, file_hash64(options->ref));
}

static double now_ms(void)
{
  static double scale;
  LARGE_INTEGER counter;
  if (scale == 0.0) {
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart == 0)
      die("QueryPerformanceFrequency failed");
    scale = 1000.0 / (double)frequency.QuadPart;
  }
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart * scale;
}

static void quote_arg(char *dst, size_t cap, const char *arg)
{
  size_t used = strlen(dst);
  int needs_quotes = strchr(arg, ' ') != 0 || strchr(arg, '\t') != 0;
  if (used + 2 >= cap) return;
  if (used) dst[used++] = ' ';
  if (needs_quotes) dst[used++] = '"';
  copy_text(dst + used, cap - used, arg);
  used = strlen(dst);
  if (needs_quotes && used + 1 < cap) { dst[used++] = '"'; dst[used] = 0; }
}

/* Run one compiler invocation.  The child's working directory is the tree root
   so relative include paths behave the same for every compiler.  Compiler
   output goes to its own log file; the timed region opens the log first so it
   covers only the child process. */
static double filetime_ms(const FILETIME *value)
{
  ULARGE_INTEGER bits;
  bits.LowPart = value->dwLowDateTime;
  bits.HighPart = value->dwHighDateTime;
  return (double)bits.QuadPart / 10000.0;
}

static double system_busy_percent(const FILETIME *idle_before, const FILETIME *kernel_before,
                                  const FILETIME *user_before, const FILETIME *idle_after,
                                  const FILETIME *kernel_after, const FILETIME *user_after)
{
  double idle = filetime_ms(idle_after) - filetime_ms(idle_before);
  double total = (filetime_ms(kernel_after) - filetime_ms(kernel_before)) +
                 (filetime_ms(user_after) - filetime_ms(user_before));
  if (total <= 0.0) return -1.0;
  return 100.0 * (total - idle) / total;
}

static int run_compiler(const char *cmdline, const char *cwd, const char *logfile,
                        double *elapsed_ms, double *cpu_ms, double *system_busy)
{
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  SECURITY_ATTRIBUTES sa;
  HANDLE null_in = INVALID_HANDLE_VALUE;
  HANDLE out = INVALID_HANDLE_VALUE;
  char *mutable_cmd;
  size_t length = strlen(cmdline) + 1;
  DWORD code = (DWORD)-1;
  BOOL started;
  double start;
  FILETIME idle_before, kernel_before, user_before;
  FILETIME idle_after, kernel_after, user_after;
  FILETIME creation, exit_time, kernel_time, user_time;

  mutable_cmd = (char *)malloc(length);
  if (!mutable_cmd) die("out of memory");
  memcpy(mutable_cmd, cmdline, length);

  sa.nLength = sizeof sa;
  sa.lpSecurityDescriptor = 0;
  sa.bInheritHandle = TRUE;
  null_in = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  out = CreateFileA(logfile, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, 0);

  memset(&si, 0, sizeof si);
  memset(&pi, 0, sizeof pi);
  si.cb = sizeof si;
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = null_in;
  si.hStdOutput = out;
  si.hStdError = out;

  /* No CREATE_NO_WINDOW here: a child with a detached console costs far more
     to start than the compiler itself does on these cases, which would swamp
     the measurement.  The worker loop runs from a console, and the standard
     handles above keep compiler output out of it. */
  start = now_ms();
  GetSystemTimes(&idle_before, &kernel_before, &user_before);
  started = CreateProcessA(0, mutable_cmd, 0, 0, TRUE, 0, 0, cwd, &si, &pi);
  if (started) {
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    if (GetProcessTimes(pi.hProcess, &creation, &exit_time, &kernel_time, &user_time))
      *cpu_ms = filetime_ms(&kernel_time) + filetime_ms(&user_time);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
  if (GetSystemTimes(&idle_after, &kernel_after, &user_after))
    *system_busy = system_busy_percent(&idle_before, &kernel_before, &user_before,
                                        &idle_after, &kernel_after, &user_after);
  *elapsed_ms = now_ms() - start;

  if (out != INVALID_HANDLE_VALUE) CloseHandle(out);
  if (null_in != INVALID_HANDLE_VALUE) CloseHandle(null_in);
  free(mutable_cmd);
  return started ? (int)code : -1;
}

static int compare_double(const void *a, const void *b)
{
  double x = *(const double *)a;
  double y = *(const double *)b;
  if (x < y) return -1;
  if (x > y) return 1;
  return 0;
}

static double median_of(const double *values, int count)
{
  double sorted[MAX_SAMPLES];
  if (count <= 0) return 0.0;
  memcpy(sorted, values, sizeof(double) * (size_t)count);
  qsort(sorted, (size_t)count, sizeof(double), compare_double);
  if (count & 1) return sorted[count / 2];
  return (sorted[count / 2 - 1] + sorted[count / 2]) / 2.0;
}

static void trim_line(char *text)
{
  size_t n = strlen(text);
  while (n > 0 && (text[n - 1] == '\r' || text[n - 1] == '\n' ||
                   text[n - 1] == ' ' || text[n - 1] == '\t'))
    text[--n] = 0;
}

static char *skip_space(char *text)
{
  while (*text == ' ' || *text == '\t') text++;
  return text;
}

static int parse_integer(const char *value, int fallback)
{
  char *end = 0;
  long parsed = strtol(value, &end, 10);
  if (end == value || parsed < 0 || parsed > 100000) return fallback;
  return (int)parsed;
}

static Case parse_case(const Options *options, const char *path)
{
  Case item;
  char *text;
  char *line;
  int line_count = 0;
  const char *base;

  memset(&item, 0, sizeof item);
  copy_text(item.path, sizeof item.path, path);
  copy_text(item.source, sizeof item.source, path);
  item.use_tcc = 1;
  item.iterations = options->iterations;
  item.warmups = options->warmups;
  base = strrchr(path, '\\');
  base = base ? base + 1 : path;
  copy_text(item.name, sizeof item.name, base);
  {
    char *dot = strrchr(item.name, '.');
    if (dot) *dot = 0;
  }

  text = read_text(path);
  if (!text) return item;
  line = text;
  while (line && *line && line_count < 24) {
    char *next = strchr(line, '\n');
    char *body;
    char *colon;
    if (next) *next = 0;
    trim_line(line);
    body = skip_space(line);
    if (body[0] == '/' && (body[1] == '/' || body[1] == '*')) body = skip_space(body + 2);
    else if (body[0] == '*') body = skip_space(body + 1);
    if (strncmp(body, "PERF_", 5) == 0 && (colon = strchr(body, ':')) != 0) {
      char *value;
      *colon = 0;
      value = skip_space(colon + 1);
      trim_line(value);
      if (strcmp(body, "PERF_NAME") == 0) copy_text(item.name, sizeof item.name, value);
      else if (strcmp(body, "PERF_ARGS") == 0) copy_text(item.args, sizeof item.args, value);
      else if (strcmp(body, "PERF_SOURCE") == 0) {
        if (value[0] == '/' || value[0] == '\\' || (value[0] && value[1] == ':'))
          copy_text(item.source, sizeof item.source, value);
        else
          join_path(item.source, sizeof item.source, options->root, value);
      }
      else if (strcmp(body, "PERF_TCC") == 0)
        item.use_tcc = !(value[0] == 'n' || value[0] == 'N' || value[0] == '0' ||
                         value[0] == 'f' || value[0] == 'F');
      else if (strcmp(body, "PERF_TIER") == 0)
        item.heavy = (value[0] == 'h' || value[0] == 'H');
      else if (strcmp(body, "PERF_ITERATIONS") == 0)
        item.iterations = parse_integer(value, item.iterations);
      else if (strcmp(body, "PERF_WARMUPS") == 0)
        item.warmups = parse_integer(value, item.warmups);
    }
    line_count++;
    if (!next) break;
    line = next + 1;
  }
  free(text);
  return item;
}

static int compare_case_name(const void *a, const void *b)
{
  const Case *x = (const Case *)a;
  const Case *y = (const Case *)b;
  return strcmp(x->name, y->name);
}

static int list_cases(const Options *options, Case *cases)
{
  char pattern[PATH_CAP];
  WIN32_FIND_DATAA entry;
  HANDLE find;
  int count = 0;

  if (!join_path(pattern, sizeof pattern, options->cases, "*.c")) die("path too long");
  find = FindFirstFileA(pattern, &entry);
  if (find == INVALID_HANDLE_VALUE) return 0;
  do {
    char full[PATH_CAP];
    if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
    if (count >= MAX_CASES) break;
    if (!join_path(full, sizeof full, options->cases, entry.cFileName)) continue;
    cases[count++] = parse_case(options, full);
  } while (FindNextFileA(find, &entry));
  FindClose(find);
  qsort(cases, (size_t)count, sizeof(Case), compare_case_name);
  return count;
}

static void build_command(char *dst, size_t cap, const char *exe, const char *bindir,
                          const Case *item, const char *out_path)
{
  char args[ARG_CAP];
  char bind_arg[PATH_CAP];
  char *cursor;

  dst[0] = 0;
  quote_arg(dst, cap, exe);
  if (bindir && bindir[0]) {
    snprintf(bind_arg, sizeof bind_arg, "-B%s", bindir);
    quote_arg(dst, cap, bind_arg);
  }
  copy_text(args, sizeof args, item->args);
  cursor = args;
  while (*cursor) {
    char *start = cursor;
    while (*start == ' ' || *start == '\t') start++;
    if (!*start) break;
    cursor = start;
    while (*cursor && *cursor != ' ' && *cursor != '\t') cursor++;
    if (*cursor) *cursor++ = 0;
    quote_arg(dst, cap, start);
  }
  quote_arg(dst, cap, "-o");
  quote_arg(dst, cap, out_path);
  quote_arg(dst, cap, item->source);
}

typedef struct {
  const char *tag;
  const char *exe;
  const char *bindir;
  int enabled;
  int failed;
  int count;
  double samples[MAX_SAMPLES];
  double median;
} RunTarget;

/* Measure every enabled compiler on one case, sample by sample rather than
   compiler by compiler.  Interleaving matters: a machine that speeds up or
   slows down halfway through the case would otherwise hand the win to whoever
   ran during the quiet half. */
static void run_case(const Options *options, const Case *item, int runs, int warmups,
                     RunTarget *targets, int target_count, int *failures, FILE *raw)
{
  int run;
  int t;

  for (t = 0; t < target_count; t++) {
    targets[t].count = 0;
    targets[t].failed = 0;
    targets[t].median = 0.0;
  }

  for (run = 0; run < runs + warmups; run++) {
    for (t = 0; t < target_count; t++) {
      RunTarget *target = &targets[t];
      char out_path[PATH_CAP];
      char log_path[PATH_CAP];
      char name[160];
      char cmd[CMD_CAP];
      double elapsed = 0.0;
      double cpu = 0.0;
      double busy = -1.0;
      int code;

      if (!target->enabled || target->failed) continue;
      snprintf(name, sizeof name, "run\\%s.exe", target->tag);
      join_path(out_path, sizeof out_path, options->out, name);
      snprintf(name, sizeof name, "logs\\%s-%s.log", target->tag, item->name);
      join_path(log_path, sizeof log_path, options->out, name);
      build_command(cmd, sizeof cmd, target->exe, target->bindir, item, out_path);

      DeleteFileA(out_path);
      code = run_compiler(cmd, options->root, log_path, &elapsed, &cpu, &busy);
      if (raw) fprintf(raw, "%s\t%s\t%d\t%d\t%.3f\t%.3f\t%.3f\t%d\t%d\n",
                       item->name, target->tag, run - warmups, run < warmups,
                       elapsed, cpu, busy, code, file_exists(out_path) ? 1 : 0);
      if (code != 0 || !file_exists(out_path)) {
        target->failed = 1;
        (*failures)++;
        continue;
      }
      if (run >= warmups && target->count < MAX_SAMPLES)
        target->samples[target->count++] = elapsed;
    }
  }
  for (t = 0; t < target_count; t++)
    targets[t].median = median_of(targets[t].samples, targets[t].count);
}

static void print_row(const char *name, const CaseResult *r)
{
  char cpc[16], tcc[16], ref[16], over_tcc[16], over_ref[16];
  if (!r->cpc_ok) copy_text(cpc, sizeof cpc, "fail");
  else snprintf(cpc, sizeof cpc, "%.2f", r->cpc_ms);
  if (!r->tcc_ok) copy_text(tcc, sizeof tcc, "-");
  else snprintf(tcc, sizeof tcc, "%.2f", r->tcc_ms);
  if (!r->ref_ok) copy_text(ref, sizeof ref, "-");
  else snprintf(ref, sizeof ref, "%.2f", r->ref_ms);
  if (r->tcc_ok) snprintf(over_tcc, sizeof over_tcc, "%.2fx", r->cpc_ms / r->tcc_ms);
  else copy_text(over_tcc, sizeof over_tcc, "-");
  if (r->ref_ok) snprintf(over_ref, sizeof over_ref, "%.2fx", r->cpc_ms / r->ref_ms);
  else copy_text(over_ref, sizeof over_ref, "-");
  printf("%-26s %9s %9s %9s %9s %9s%s\n", name, cpc, tcc, ref, over_tcc, over_ref,
         r->cpc_ok ? "" : "  FAIL");
}

typedef struct {
  char name[96];
  double cpc_ms;
  double cpc_over_tcc;
  double cpc_over_ref;
} BaselineRow;

static int load_baseline(const char *path, BaselineRow *rows, int max)
{
  char *text = read_text(path);
  char *line;
  int count = 0;
  if (!text) return 0;
  line = text;
  while (line && *line) {
    char *next = strchr(line, '\n');
    char *name;
    char *f1;
    char *f2;
    char *f3;
    if (next) *next = 0;
    trim_line(line);
    if (line[0] && line[0] != '#') {
      name = line;
      f1 = strchr(name, '\t');
      if (f1) {
        *f1++ = 0;
        f2 = strchr(f1, '\t');
        if (f2) *f2++ = 0;
        f3 = f2 ? strchr(f2, '\t') : 0;
        if (f3) *f3++ = 0;
        if (count < max) {
          memset(&rows[count], 0, sizeof rows[count]);
          copy_text(rows[count].name, sizeof rows[count].name, name);
          rows[count].cpc_ms = atof(f1);
          rows[count].cpc_over_tcc = f2 ? atof(f2) : 0.0;
          rows[count].cpc_over_ref = f3 ? atof(f3) : 0.0;
          count++;
        }
      }
    }
    if (!next) break;
    line = next + 1;
  }
  free(text);
  return count;
}

static BaselineRow *find_baseline(BaselineRow *rows, int count, const char *name)
{
  int i;
  for (i = 0; i < count; i++)
    if (strcmp(rows[i].name, name) == 0) return &rows[i];
  return 0;
}

static void timestamp_utc(char *dst, size_t cap)
{
  time_t now = time(0);
  struct tm *g = gmtime(&now);
  if (g) strftime(dst, cap, "%Y-%m-%dT%H:%M:%SZ", g);
  else copy_text(dst, cap, "-");
}

static int read_last_cycle(const char *path)
{
  char *text = read_text(path);
  char *line;
  int last = 0;
  if (!text) return 0;
  line = text;
  while (line && *line) {
    char *next = strchr(line, '\n');
    if (next) *next = 0;
    if (line[0] && line[0] != '#') {
      char *f1 = strchr(line, '\t');
      if (f1) {
        char *f2 = strchr(f1 + 1, '\t');
        if (f2) {
          int cycle = atoi(f2 + 1);
          if (cycle > last) last = cycle;
        }
      }
    }
    if (!next) break;
    line = next + 1;
  }
  free(text);
  return last;
}

/* Print the last `keep` sample rows of the performance log, oldest first, so a
   worker cycle can show the trend without parsing the file in batch. */
static void print_history(const char *path, int keep)
{
  char *text = read_text(path);
  char *line;
  static char rows[32][256];
  int count = 0;
  int i;
  if (!text) {
    printf("no performance log at %s\n", path);
    return;
  }
  if (keep > 32) keep = 32;
  if (keep < 1) keep = 1;
  line = text;
  while (line && *line) {
    char *next = strchr(line, '\n');
    if (next) *next = 0;
    trim_line(line);
    if (line[0] && line[0] != '#') {
      if (count >= keep) {
        int slot;
        for (slot = 0; slot + 1 < keep; slot++) copy_text(rows[slot], 256, rows[slot + 1]);
        count = keep - 1;
      }
      copy_text(rows[count], 256, line);
      count++;
    }
    if (!next) break;
    line = next + 1;
  }
  free(text);
  if (count == 0) {
    printf("no samples recorded yet in %s\n", path);
    return;
  }
  printf("last %d sample(s) in %s\n", count, path);
  for (i = 0; i < count; i++) {
    char work[256];
    char *fields[8];
    int f = 0;
    char *cursor;
    copy_text(work, sizeof work, rows[i]);
    fields[f++] = work;
    for (cursor = work; *cursor && f < 8; cursor++)
      if (*cursor == '\t') { *cursor = 0; fields[f++] = cursor + 1; }
    /* utc cycle event value1 value2 value3 head */
    printf("  %s  cycle %s  %-7s value1 %s value2 %s value3 %s\n",
           fields[0], f > 2 ? fields[2] : "-", f > 3 ? fields[3] : "-",
           f > 4 ? fields[4] : "-", f > 5 ? fields[5] : "-", f > 6 ? fields[6] : "-");
  }
}

static void parse_options(int argc, char **argv, Options *options)
{
  int i;

  memset(options, 0, sizeof *options);
  copy_text(options->root, sizeof options->root, ".");
  options->iterations = 5;
  options->warmups = 1;
  options->tolerance = 25.0;

  for (i = 1; i < argc; i++) {
    const char *arg = argv[i];
    const char *next = (i + 1 < argc) ? argv[i + 1] : 0;
    if (strcmp(arg, "-Root") == 0 && next) { copy_text(options->root, sizeof options->root, next); i++; }
    else if (strcmp(arg, "-Cpc") == 0 && next) { copy_text(options->cpc, sizeof options->cpc, next); i++; }
    else if (strcmp(arg, "-Tcc") == 0 && next) { copy_text(options->tcc, sizeof options->tcc, next); i++; }
    else if (strcmp(arg, "-Reference") == 0 && next) { copy_text(options->ref, sizeof options->ref, next); i++; }
    else if (strcmp(arg, "-Cases") == 0 && next) { copy_text(options->cases, sizeof options->cases, next); i++; }
    else if (strcmp(arg, "-Out") == 0 && next) { copy_text(options->out, sizeof options->out, next); i++; }
    else if (strcmp(arg, "-Results") == 0 && next) { copy_text(options->results, sizeof options->results, next); i++; }
    else if (strcmp(arg, "-RawSamples") == 0 && next) { copy_text(options->raw_samples, sizeof options->raw_samples, next); i++; }
    else if (strcmp(arg, "-Baseline") == 0 && next) { copy_text(options->baseline, sizeof options->baseline, next); i++; }
    else if (strcmp(arg, "-Log") == 0 && next) { copy_text(options->log, sizeof options->log, next); i++; }
    else if (strcmp(arg, "-Head") == 0 && next) { copy_text(options->head, sizeof options->head, next); i++; }
    else if (strcmp(arg, "-Iterations") == 0 && next) { options->iterations = parse_integer(next, options->iterations); i++; }
    else if (strcmp(arg, "-Warmups") == 0 && next) { options->warmups = parse_integer(next, options->warmups); i++; }
    else if (strcmp(arg, "-Cycle") == 0 && next) { options->cycle = parse_integer(next, 0); i++; }
    else if (strcmp(arg, "-Tolerance") == 0 && next) { options->tolerance = atof(next); i++; }
    else if (strcmp(arg, "-UpdateBaseline") == 0) options->update_baseline = 1;
    else if (strcmp(arg, "-NoGate") == 0) options->no_gate = 1;
    else if (strcmp(arg, "-Fast") == 0) options->fast = 1;
    else if (strcmp(arg, "-Quiet") == 0) options->quiet = 1;
    else if (strcmp(arg, "-LastCycle") == 0) options->last_cycle = 1;
    else if (strcmp(arg, "-History") == 0) { options->history = next ? parse_integer(next, 5) : 5; if (next) i++; }
    else if (strcmp(arg, "-Help") == 0 || strcmp(arg, "-h") == 0) { printf("%s", help_text); exit(0); }
    else { fprintf(stderr, "perf-compare: unknown option '%s'\n%s", arg, help_text); exit(2); }
  }

  absolute_path(options->root, sizeof options->root, options->root);
  if (!options->cpc[0]) join_path(options->cpc, sizeof options->cpc, options->root, "cpc.exe");
  if (!options->tcc[0]) join_path(options->tcc, sizeof options->tcc, options->root, "third-party\\tcc\\win32\\tcc.exe");
  if (!options->cases[0]) join_path(options->cases, sizeof options->cases, options->root, "Tests\\benchmarks\\compile");
  if (!options->out[0]) join_path(options->out, sizeof options->out, options->root, "build\\perf");
  if (!options->results[0]) join_path(options->results, sizeof options->results, options->out, "perf-results.tsv");
  if (!options->raw_samples[0]) join_path(options->raw_samples, sizeof options->raw_samples, options->out, "perf-samples.tsv");
  if (!options->baseline[0]) join_path(options->baseline, sizeof options->baseline, options->root, "Performance\\baseline\\perf-baseline.tsv");
  absolute_path(options->cpc, sizeof options->cpc, options->cpc);
  absolute_path(options->tcc, sizeof options->tcc, options->tcc);
  absolute_path(options->cases, sizeof options->cases, options->cases);
  absolute_path(options->out, sizeof options->out, options->out);
  absolute_path(options->results, sizeof options->results, options->results);
  absolute_path(options->raw_samples, sizeof options->raw_samples, options->raw_samples);
  absolute_path(options->baseline, sizeof options->baseline, options->baseline);
  if (options->log[0]) absolute_path(options->log, sizeof options->log, options->log);
  options->have_ref = options->ref[0] != 0;
  if (options->have_ref) absolute_path(options->ref, sizeof options->ref, options->ref);
}

static void tcc_bindir(const Options *options, char *dst, size_t cap)
{
  copy_text(dst, cap, options->tcc);
  {
    char *cut = strrchr(dst, '\\');
    if (cut) *cut = 0;
  }
}

int main(int argc, char **argv)
{
  Options options;
  Case cases[MAX_CASES];
  CaseResult results[MAX_CASES];
  BaselineRow baseline[MAX_CASES];
  int baseline_count;
  char tmp_dir[PATH_CAP];
  char log_dir[PATH_CAP];
  char tcc_bin[PATH_CAP];
  int case_count;
  int ran = 0;
  int i;
  int failures = 0;
  int violations = 0;
  double total_cpc_fast = 0.0, total_tcc_fast = 0.0;
  double total_cpc_all = 0.0, total_tcc_all = 0.0, total_ref_all = 0.0;
  double cpc_only_all = 0.0;
  int tcc_cases = 0, ref_cases = 0, missing_matched = 0;
  int heavy_cpc_ms = 0;
  FILE *raw;

  parse_options(argc, argv, &options);
  if (options.last_cycle) { printf("%d\n", read_last_cycle(options.log)); return 0; }
  if (options.history > 0) { print_history(options.log, options.history); return 0; }

  if (!file_exists(options.cpc)) die("compiler under test not found");
  if (!directory_exists(options.cases)) die("case directory not found");
  if (options.have_ref && !file_exists(options.ref)) die("reference compiler not found");
  if (!make_directory(options.out)) die("cannot create output directory");
  join_path(tmp_dir, sizeof tmp_dir, options.out, "run");
  join_path(log_dir, sizeof log_dir, options.out, "logs");
  if (!make_directory(tmp_dir) || !make_directory(log_dir))
    die("cannot create output directories");
  raw = fopen(options.raw_samples, "wb");
  if (!raw) die("cannot write raw samples");
  fprintf(raw, "# cprime perf samples v2\n");
  write_runtime_identity(raw, &options);
  fprintf(raw, "# case\tcompiler\tsample\twarmup\twall_ms\tchild_cpu_ms\tsystem_busy_percent\texit\toutput\n");
  tcc_bindir(&options, tcc_bin, sizeof tcc_bin);

  case_count = list_cases(&options, cases);
  if (case_count == 0) die("no C cases found");
  for (i = 0; i < case_count; i++)
    fprintf(raw, "# input\t%s\t%s\tfnv1a64=%016llx\tflags=%s\n", cases[i].name,
            cases[i].source, file_hash64(cases[i].source), cases[i].args[0] ? cases[i].args : "(none)");

  printf("CPrime C compilation speed\n");
  printf("  cpc       : %s\n", options.cpc);
  if (file_exists(options.tcc)) printf("  tcc       : %s\n", options.tcc);
  else printf("  tcc       : (not found; tcc rows skipped)\n");
  if (options.have_ref) printf("  reference : %s\n", options.ref);
  printf("  cases     : %s\n", options.cases);
  if (!options.quiet)
    printf("\n%-26s %9s %9s %9s %9s %9s\n", "case", "cpc ms", "tcc ms", "ref ms",
           "cpc/tcc", "cpc/ref");

  memset(results, 0, sizeof results);
  for (i = 0; i < case_count; i++) {
    const Case *item = &cases[i];
    CaseResult *r = &results[i];
    RunTarget targets[3];
    int target_count = 0;
    int iterations = item->iterations > 0 ? item->iterations : 1;

    if (options.fast && item->heavy) continue;
    ran++;

    memset(targets, 0, sizeof targets);
    targets[target_count].tag = "cpc";
    targets[target_count].exe = options.cpc;
    targets[target_count].enabled = 1;
    target_count++;
    if (options.have_ref) {
      targets[target_count].tag = "reference";
      targets[target_count].exe = options.ref;
      targets[target_count].enabled = 1;
      target_count++;
    }
    if (item->use_tcc && file_exists(options.tcc)) {
      targets[target_count].tag = "tcc";
      targets[target_count].exe = options.tcc;
      targets[target_count].bindir = tcc_bin;
      targets[target_count].enabled = 1;
      target_count++;
    }

    run_case(&options, item, iterations, item->warmups, targets, target_count, &failures, raw);

    if (targets[0].count > 0) { r->cpc_ok = 1; r->cpc_ms = targets[0].median; }
    if (options.have_ref && !targets[1].failed && targets[1].count > 0) {
      r->ref_ok = 1;
      r->ref_ms = targets[1].median;
    }
    {
      int tcc_index = options.have_ref ? 2 : 1;
      if (tcc_index < target_count && targets[tcc_index].count > 0) {
        r->tcc_ok = 1;
        r->tcc_ms = targets[tcc_index].median;
      }
    }

    if (!options.quiet) print_row(item->name, r);

    if (r->cpc_ok && r->tcc_ok) {
      total_cpc_all += r->cpc_ms;
      if (!item->heavy) total_cpc_fast += r->cpc_ms;
      total_tcc_all += r->tcc_ms;
      if (!item->heavy) total_tcc_fast += r->tcc_ms;
      tcc_cases++;
    } else if (item->use_tcc) {
      /* A comparison row is unusable unless both compilers measured it. */
      missing_matched++;
    }
    if (r->cpc_ok && r->ref_ok) { total_ref_all += r->ref_ms; ref_cases++; }
    if (r->cpc_ok && !item->use_tcc) {
      cpc_only_all += r->cpc_ms;
      if (item->heavy) heavy_cpc_ms = (int)(r->cpc_ms + 0.5);
    }
  }

  fclose(raw);

  if (!options.quiet) {
    printf("\n");
    printf("fast tier  : cpc %8.2f ms   tcc %8.2f ms", total_cpc_fast, total_tcc_fast);
    if (total_tcc_fast > 0.0)
      printf("   cpc takes %.2fx tcc time (%.0f%% of tcc throughput)",
             total_cpc_fast / total_tcc_fast, 100.0 * total_tcc_fast / total_cpc_fast);
    printf("\n");
    printf("all matched: cpc %8.2f ms   tcc %8.2f ms", total_cpc_all, total_tcc_all);
    if (total_tcc_all > 0.0) printf("   cpc takes %.2fx tcc time", total_cpc_all / total_tcc_all);
    printf("\n");
    printf("cpc-only   : %8.2f ms (excluded from CPC/TCC ratios)\n", cpc_only_all);
    if (ref_cases > 0) {
      printf("reference  : %8.2f ms across %d matched case(s)", total_ref_all, ref_cases);
      if (total_ref_all > 0.0)
        printf("   cpc takes %.3fx reference time", total_cpc_all / total_ref_all);
      printf("\n");
    }
  }

  {
    FILE *out = fopen(options.results, "wb");
    if (out) {
      fprintf(out, "# cprime perf results v1\n");
      fprintf(out, "# case\tcompiler\tmedian_ms\tfailed\n");
      for (i = 0; i < case_count; i++) {
        const Case *item = &cases[i];
        const CaseResult *r = &results[i];
        if (!r->cpc_ok && !r->tcc_ok && !r->ref_ok) continue;
        fprintf(out, "%s\tcpc\t%.3f\t%d\n", item->name, r->cpc_ms, r->cpc_ok ? 0 : 1);
        if (options.have_ref) fprintf(out, "%s\treference\t%.3f\t%d\n", item->name, r->ref_ms, r->ref_ok ? 0 : 1);
        if (item->use_tcc) fprintf(out, "%s\ttcc\t%.3f\t%d\n", item->name, r->tcc_ms, r->tcc_ok ? 0 : 1);
      }
      fprintf(out, "# fast_cpc_ms\t%.3f\n", total_cpc_fast);
      fprintf(out, "# fast_tcc_ms\t%.3f\n", total_tcc_fast);
      fclose(out);
    }
  }

  baseline_count = load_baseline(options.baseline, baseline, MAX_CASES);

  if (options.update_baseline) {
    FILE *out = fopen(options.baseline, "wb");
    if (!out) die("cannot write baseline");
    fprintf(out, "# cprime perf baseline v1\n");
    fprintf(out, "# case\tcpc_ms\tcpc_over_tcc\tcpc_over_ref\n");
    for (i = 0; i < case_count; i++) {
      const Case *item = &cases[i];
      const CaseResult *r = &results[i];
      double over_tcc = r->tcc_ok ? r->cpc_ms / r->tcc_ms : 0.0;
      double over_ref = r->ref_ok ? r->cpc_ms / r->ref_ms : 0.0;
      if (!r->cpc_ok) continue;
      fprintf(out, "%s\t%.3f\t%.4f\t%.4f\n", item->name, r->cpc_ms, over_tcc, over_ref);
    }
    fclose(out);
    printf("baseline written: %s\n", options.baseline);
  } else if (!options.no_gate && ran > 0) {
    if (baseline_count == 0)
      printf("baseline: %s (absent; record one with -UpdateBaseline)\n", options.baseline);
    else
      printf("baseline: %s (tolerance %.0f%%)\n", options.baseline, options.tolerance);
    for (i = 0; i < case_count; i++) {
      const Case *item = &cases[i];
      const CaseResult *r = &results[i];
      BaselineRow *base;
      double current = 0.0, recorded = 0.0, limit, delta;
      const char *kind = 0;
      int bad;
      if (!r->cpc_ok) continue;
      base = find_baseline(baseline, baseline_count, item->name);
      if (!base) continue;
      if (r->tcc_ok && base->cpc_over_tcc > 0.0) {
        current = r->cpc_ms / r->tcc_ms; recorded = base->cpc_over_tcc; kind = "cpc/tcc";
      } else if (r->ref_ok && base->cpc_over_ref > 0.0) {
        current = r->cpc_ms / r->ref_ms; recorded = base->cpc_over_ref; kind = "cpc/ref";
      } else if (base->cpc_ms > 0.0) {
        current = r->cpc_ms; recorded = base->cpc_ms; kind = "cpc ms";
      }
      if (!kind) continue;
      limit = recorded * (1.0 + options.tolerance / 100.0);
      delta = 100.0 * (current / recorded - 1.0);
      bad = current > limit;
      if (bad) violations++;
      printf("  %-24s %-8s %8.3f vs %8.3f (%+6.1f%%) %s\n", item->name, kind, current,
             recorded, delta, bad ? "REGRESSION" : "ok");
    }
  }

  if (options.log[0]) {
    FILE *out = fopen(options.log, "ab");
    if (out) {
      char stamp[64];
      double ratio = total_tcc_fast > 0.0 ? total_cpc_fast / total_tcc_fast : 0.0;
      timestamp_utc(stamp, sizeof stamp);
      fprintf(out, "%s\t%lld\t%d\tperf\t%.4f\t%d\t%d\t%s\n", stamp, (long long)time(0),
              options.cycle, ratio, heavy_cpc_ms, failures,
              options.head[0] ? options.head : "-");
      fclose(out);
    }
  }

  if (failures > 0) {
    printf("%d compiler invocation(s) failed; see %s\\logs\n", failures, options.out);
    return 1;
  }
  if (missing_matched > 0) {
    printf("%d shared CPC/TCC case(s) did not produce a matched measurement\n", missing_matched);
    return 1;
  }
  if (violations > 0) {
    printf("%d speed regression(s) beyond the %.0f%% tolerance\n", violations, options.tolerance);
    return 1;
  }
  return 0;
}
