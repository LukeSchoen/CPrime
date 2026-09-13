/* Scan the first-party compiler sources for leftovers that make compilation
   slower or the tree harder to keep clean, and compare the counts with a
   recorded baseline.

   The checks are deliberately cheap textual ones: they are meant to run on
   every worker cycle, not to replace reading the code.  Two classes matter:

     environment and process lookups.  getenv() scans the whole environment
     block on every call, and GetEnvironmentVariable/GetModuleFileName/
     RegOpenKey are registry or loader backed.  One call on a debug path is
     harmless; one call per token or per lookup is not.  Their counts may only
     fall.

     leftovers.  TODO/FIXME/HACK/XXX markers, #if 0 disabled code and stray
     scratch, probe, tmp, .orig, .rej or .bak files are work that was never
     finished.  New ones fail the run; existing ones stay visible in the
     report so they can be retired deliberately.

   Usage: perf-check [options]
     -Root DIR        tree root (default ".")
     -Baseline FILE   baseline TSV (default ROOT/Performance/baseline/checks.tsv)
     -Out FILE        current counts TSV (default ROOT/build/perf/perf-checks.tsv)
     -UpdateBaseline  record the current counts and do not gate them
     -NoGate          report only; never fail on a new finding
     -Log FILE        append one summary row to a shared performance log
     -Cycle N         cycle number recorded in -Log rows
     -Head REV        commit hash recorded in -Log rows
     -Quiet           print only the findings and the summary
     -Help            print this text

   Exit: 0 ok, 1 new finding, 2 usage or input error. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <windows.h>

#define MAX_PATTERNS 24
#define MAX_STRAYS 128
#define PATH_CAP 512

typedef struct {
  const char *id;
  const char *needle;
  const char *note;
} Pattern;

static const Pattern patterns[] = {
  { "env.win",       "GetEnvironmentVariable", "registry backed lookup; cache it once" },
  { "env.crt",       "getenv(",               "scans the environment block on every call" },
  { "env.expand",    "ExpandEnvironmentStrings", "environment walk" },
  { "env.module",    "GetModuleFileName",     "loader lookup; resolve once" },
  { "env.registry",  "RegOpenKey",            "registry access" },
  { "marker.todo",   "TODO",                  "unfinished work" },
  { "marker.fixme",  "FIXME",                 "known defect left in place" },
  { "marker.hack",   "HACK",                  "workaround left in place" },
  { "marker.xxx",    "XXX",                   "undecided or unfinished code" },
  { "dead.if0",      "#if 0",                 "disabled code still in the tree" }
};

static const int pattern_count = (int)(sizeof patterns / sizeof patterns[0]);

typedef struct {
  char id[64];
  int count;
} Row;

static const char help_text[] =
  "usage: perf-check [options]\n"
  "  -Root DIR        tree root (default \".\")\n"
  "  -Baseline FILE   baseline TSV (default ROOT/Performance/baseline/checks.tsv)\n"
  "  -Out FILE        current counts TSV (default ROOT/build/perf/perf-checks.tsv)\n"
  "  -UpdateBaseline  record the current counts\n"
  "  -NoGate          report only; never fail on a new finding\n"
  "  -Log FILE        append one summary row to a shared performance log\n"
  "  -Cycle N         cycle number recorded in -Log rows\n"
  "  -Head REV        commit hash recorded in -Log rows\n"
  "  -Quiet           print only findings and the summary\n";

static void die(const char *message)
{
  fprintf(stderr, "perf-check: %s\n", message);
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

static int directory_exists(const char *path)
{
  DWORD attrs = GetFileAttributesA(path);
  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
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
  if (size < 0 || size > 16L * 1024 * 1024) { fclose(file); return 0; }
  if (fseek(file, 0, SEEK_SET) != 0) { fclose(file); return 0; }
  text = (char *)malloc((size_t)size + 2);
  if (!text) { fclose(file); return 0; }
  got = fread(text, 1, (size_t)size, file);
  fclose(file);
  text[got] = 0;
  return text;
}

static int count_occurrences(const char *text, const char *needle)
{
  size_t length = strlen(needle);
  const char *cursor = text;
  int count = 0;
  if (!length) return 0;
  while ((cursor = strstr(cursor, needle)) != 0) { count++; cursor += length; }
  return count;
}

static int has_source_extension(const char *name)
{
  const char *dot = strrchr(name, '.');
  if (!dot) return 0;
  return strcmp(dot, ".c") == 0 || strcmp(dot, ".h") == 0 || strcmp(dot, ".inc") == 0 ||
         strcmp(dot, ".cpp") == 0 || strcmp(dot, ".S") == 0 || strcmp(dot, ".cc") == 0;
}

static int name_is_stray(const char *name)
{
  size_t length = strlen(name);
  const char *needles[3];
  int i;
  needles[0] = "scratch";
  needles[1] = "probe";
  needles[2] = "tmp";
  for (i = 0; i < 3; i++) {
    const char *found = strstr(name, needles[i]);
    while (found) {
      /* Only flag the word, so "tmp" matches "tmp_x" but not "attempt". */
      int before_ok = (found == name) || !isalnum((unsigned char)found[-1]);
      int after_ok = (found[strlen(needles[i])] == 0) ||
                     !isalnum((unsigned char)found[strlen(needles[i])]);
      if (before_ok && after_ok) return 1;
      found = strstr(found + 1, needles[i]);
    }
  }
  if (length > 1 && name[length - 1] == '~') return 1;
  if (length > 5 && strcmp(name + length - 5, ".orig") == 0) return 1;
  if (length > 4 && strcmp(name + length - 4, ".rej") == 0) return 1;
  if (length > 4 && strcmp(name + length - 4, ".bak") == 0) return 1;
  return 0;
}

typedef struct {
  char path[PATH_CAP];
} StrayFile;

typedef struct {
  const char *root;
  int counts[MAX_PATTERNS];
  int worst[MAX_PATTERNS];
  char worst_path[MAX_PATTERNS][PATH_CAP];
  int files;
  long long bytes;
  StrayFile strays[MAX_STRAYS];
  int stray_count;
} Scan;

/* Scanning is iterative but the per-file table is not small; keep it off the
   stack. */
static Scan scan;

static void scan_file(Scan *scan, const char *path, const char *relative)
{
  char *text = read_text(path);
  int i;
  if (!text) return;
  scan->files++;
  scan->bytes += (long long)strlen(text);
  for (i = 0; i < pattern_count; i++) {
    int count = count_occurrences(text, patterns[i].needle);
    if (count <= 0) continue;
    scan->counts[i] += count;
    if (count > scan->worst[i]) {
      scan->worst[i] = count;
      copy_text(scan->worst_path[i], PATH_CAP, relative);
    }
  }
  free(text);
}

static void scan_directory(Scan *scan, const char *dir, const char *relative_prefix)
{
  char pattern[PATH_CAP];
  WIN32_FIND_DATAA entry;
  HANDLE find;

  if (!join_path(pattern, sizeof pattern, dir, "*")) return;
  find = FindFirstFileA(pattern, &entry);
  if (find == INVALID_HANDLE_VALUE) return;
  do {
    char full[PATH_CAP];
    char relative[PATH_CAP];
    if (strcmp(entry.cFileName, ".") == 0 || strcmp(entry.cFileName, "..") == 0) continue;
    join_path(full, sizeof full, dir, entry.cFileName);
    if (relative_prefix[0]) snprintf(relative, sizeof relative, "%s/%s", relative_prefix, entry.cFileName);
    else copy_text(relative, sizeof relative, entry.cFileName);
    if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      scan_directory(scan, full, relative);
      continue;
    }
    if (!has_source_extension(entry.cFileName)) continue;
    scan_file(scan, full, relative);
    if (name_is_stray(entry.cFileName) && scan->stray_count < MAX_STRAYS) {
      copy_text(scan->strays[scan->stray_count].path, PATH_CAP, relative);
      scan->stray_count++;
    }
  } while (FindNextFileA(find, &entry));
  FindClose(find);
}

static int baseline_lookup(const Row *rows, int count, const char *id, int fallback)
{
  int i;
  for (i = 0; i < count; i++)
    if (strcmp(rows[i].id, id) == 0) return rows[i].count;
  return fallback;
}

static int load_baseline(const char *path, Row *rows, int max)
{
  char *text = read_text(path);
  char *line;
  int count = 0;
  if (!text) return 0;
  line = text;
  while (line && *line) {
    char *next = strchr(line, '\n');
    char *tab;
    size_t n;
    if (next) *next = 0;
    n = strlen(line);
    while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == ' ' || line[n - 1] == '\t'))
      line[--n] = 0;
    if (line[0] && line[0] != '#' && (tab = strchr(line, '\t')) != 0 && count < max) {
      *tab = 0;
      memset(&rows[count], 0, sizeof rows[count]);
      copy_text(rows[count].id, sizeof rows[count].id, line);
      rows[count].count = atoi(tab + 1);
      count++;
    }
    if (!next) break;
    line = next + 1;
  }
  free(text);
  return count;
}

static void timestamp_utc(char *dst, size_t cap)
{
  time_t now = time(0);
  struct tm *g = gmtime(&now);
  if (g) strftime(dst, cap, "%Y-%m-%dT%H:%M:%SZ", g);
  else copy_text(dst, cap, "-");
}

int main(int argc, char **argv)
{
  char root[PATH_CAP] = ".";
  char baseline_path[PATH_CAP] = "";
  char out_path[PATH_CAP] = "";
  char log_path[PATH_CAP] = "";
  char head[64] = "";
  int update_baseline = 0;
  int no_gate = 0;
  int quiet = 0;
  int cycle = 0;
  int i;
  Row baseline[MAX_PATTERNS + 4];
  int baseline_rows;
  int violations = 0;
  int have_baseline = 0;
  int stray_over = 0;
  int cpc_only_note = 0;

  for (i = 1; i < argc; i++) {
    const char *arg = argv[i];
    const char *next = (i + 1 < argc) ? argv[i + 1] : 0;
    if (strcmp(arg, "-Root") == 0 && next) { copy_text(root, sizeof root, next); i++; }
    else if (strcmp(arg, "-Baseline") == 0 && next) { copy_text(baseline_path, sizeof baseline_path, next); i++; }
    else if (strcmp(arg, "-Out") == 0 && next) { copy_text(out_path, sizeof out_path, next); i++; }
    else if (strcmp(arg, "-Log") == 0 && next) { copy_text(log_path, sizeof log_path, next); i++; }
    else if (strcmp(arg, "-Head") == 0 && next) { copy_text(head, sizeof head, next); i++; }
    else if (strcmp(arg, "-Cycle") == 0 && next) { cycle = atoi(next); i++; }
    else if (strcmp(arg, "-UpdateBaseline") == 0) update_baseline = 1;
    else if (strcmp(arg, "-NoGate") == 0) no_gate = 1;
    else if (strcmp(arg, "-Quiet") == 0) quiet = 1;
    else if (strcmp(arg, "-Help") == 0 || strcmp(arg, "-h") == 0) { printf("%s", help_text); return 0; }
    else { fprintf(stderr, "perf-check: unknown option '%s'\n%s", arg, help_text); return 2; }
  }
  (void)cpc_only_note;

  absolute_path(root, sizeof root, root);
  if (!baseline_path[0]) join_path(baseline_path, sizeof baseline_path, root, "Performance\\baseline\\checks.tsv");
  if (!out_path[0]) join_path(out_path, sizeof out_path, root, "build\\perf\\perf-checks.tsv");
  absolute_path(baseline_path, sizeof baseline_path, baseline_path);
  absolute_path(out_path, sizeof out_path, out_path);
  if (log_path[0]) absolute_path(log_path, sizeof log_path, log_path);

  memset(&scan, 0, sizeof scan);
  scan.root = root;
  for (i = 0; i < 2; i++) {
    char dir[PATH_CAP];
    join_path(dir, sizeof dir, root, i == 0 ? "src" : "include");
    if (directory_exists(dir)) scan_directory(&scan, dir, i == 0 ? "src" : "include");
  }
  if (scan.files == 0) die("no first-party sources found under src/ or include/");

  baseline_rows = load_baseline(baseline_path, baseline, MAX_PATTERNS + 4);
  have_baseline = baseline_rows > 0;

  printf("CPrime first-party leftovers and lookups\n");
  printf("  scanned   : %d files, %lld bytes under src/ and include/\n", scan.files, scan.bytes);
  printf("  baseline  : %s%s\n", baseline_path, have_baseline ? "" : " (absent)");
  if (!quiet) {
    printf("\n%-16s %7s %9s %7s  %s\n", "check", "count", "baseline", "delta", "note");
    for (i = 0; i < pattern_count; i++) {
      int recorded = have_baseline ? baseline_lookup(baseline, baseline_rows, patterns[i].id, 0) : 0;
      int delta = scan.counts[i] - recorded;
      printf("%-16s %7d %9d %+7d  %s\n", patterns[i].id, scan.counts[i], recorded, delta,
             patterns[i].note);
      if (scan.counts[i] > 0 && scan.worst_path[i][0])
        printf("  %-14s most in %s (%d)\n", "", scan.worst_path[i], scan.worst[i]);
      if (have_baseline && delta > 0 && !update_baseline) violations++;
    }
    {
      int recorded = have_baseline ? baseline_lookup(baseline, baseline_rows, "stray.files", 0) : 0;
      printf("%-16s %7d %9d %+7d  %s\n", "stray.files", scan.stray_count, recorded,
             scan.stray_count - recorded, "scratch/probe/tmp/.orig/.rej/.bak files in first-party trees");
      for (i = 0; i < scan.stray_count; i++)
        printf("  %-14s %s\n", "", scan.strays[i].path);
      if (have_baseline && scan.stray_count > recorded && !update_baseline) {
        violations++;
        stray_over = 1;
      }
    }
  }

  {
    FILE *out = fopen(out_path, "wb");
    if (out) {
      fprintf(out, "# cprime perf checks v1\n");
      fprintf(out, "# id\tcount\n");
      for (i = 0; i < pattern_count; i++) fprintf(out, "%s\t%d\n", patterns[i].id, scan.counts[i]);
      fprintf(out, "stray.files\t%d\n", scan.stray_count);
      fclose(out);
    }
  }

  if (update_baseline) {
    FILE *out = fopen(baseline_path, "wb");
    if (!out) die("cannot write baseline");
    fprintf(out, "# cprime checks baseline v1\n");
    fprintf(out, "# id\tcount\n");
    for (i = 0; i < pattern_count; i++) fprintf(out, "%s\t%d\n", patterns[i].id, scan.counts[i]);
    fprintf(out, "stray.files\t%d\n", scan.stray_count);
    fclose(out);
    printf("baseline written: %s\n", baseline_path);
    violations = 0;
  } else if (!have_baseline) {
    printf("no baseline recorded; run with -UpdateBaseline to gate future changes\n");
  } else {
    printf("%d new leftover/lookup finding(s)%s\n", violations,
           stray_over ? " (including stray files)" : "");
  }

  if (log_path[0]) {
    FILE *out = fopen(log_path, "ab");
    if (out) {
      char stamp[64];
      timestamp_utc(stamp, sizeof stamp);
      fprintf(out, "%s\t%lld\t%d\tchecks\t%d\t%d\t%d\t%s\n", stamp, (long long)time(0), cycle,
              violations, pattern_count, scan.stray_count, head[0] ? head : "-");
      fclose(out);
    }
  }

  if (violations > 0 && !no_gate) return 1;
  return 0;
}
