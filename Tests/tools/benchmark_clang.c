/* Explicit external-compiler experiment; never part of the regression gate.
   One warmup, interleaved measured runs, one inherited CPU affinity bit.
   Logs retain exact commands, diagnostics, exit status and output size. */
#include "../../scripts/common/native_tool.h"

typedef struct Case { const char *name, *source; int self, cpp, stl; } Case;
static const Case cases[] = {
    {"empty_cpp", "Tests/benchmarks/compile/competitive/empty.cpp", 0, 1, 0},
    {"template_lookup", "Tests/benchmarks/compile/test_static_template_lookup.cpp", 0, 1, 0},
    {"template_conversion", "Tests/benchmarks/compile/test_conversion_template_owner_lookup.cpp", 0, 1, 0},
    {"cpp_functions", "Tests/benchmarks/compile/competitive/functions.cpp", 0, 1, 0},
    {"stl_vector", "Tests/features/Templates/pass/test_std_vector_nested.cpp", 0, 1, 1},
    {"xbrz", "Tests/benchmarks/compile/competitive/xbrz_check.cpp", 0, 1, 1},
    {"compiler_c", "src/compiler/driver/cprime.c", 1, 0, 0}
};
static const char *modes[] = {"cpc_self", "cpc_clang", "clang_O0", "clang_fast", "minimal_fast", "clang_pch", "minimal_pch"};
#define MODE_COUNT 7
static const char *phases[] = {"syntax", "object", "exe"};
static char root[NT_PATH], out[NT_PATH], cpc[NT_PATH], fast_cpc[NT_PATH], clang[NT_PATH], minimal[NT_PATH];
static FILE *raw, *commands;
static HANDLE accounting;
static char *expected_stdout[sizeof cases/sizeof cases[0]];
static int output_mismatch[sizeof cases/sizeof cases[0]];
static double tree_cpu(void);
static unsigned long long file_bytes(const char *path);
static const char *clang_fast_flags[] = {"-fintegrated-cc1", "-fno-crash-diagnostics", "-fno-exceptions", "-fno-rtti", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables", "-fno-stack-protector", "-fno-addrsig", NULL};
static int is_minimal(int mode) { return mode==4 || mode==6; }

static int prepare_pch(int mode) {
    const char *args[32]; int n=0, i; char output[NT_PATH], log[NT_PATH];
    NtBuffer line={0}; NtProcessResult r; double cpu;
    snprintf(output,sizeof output,"%s/%s.pch",out,modes[mode]);
    args[n++]=is_minimal(mode) ? minimal : clang;
    args[n++]="-O0"; args[n++]="-g0"; args[n++]="-std=c++17";
    for(i=0; clang_fast_flags[i]; ++i) args[n++]=clang_fast_flags[i];
    args[n++]="-x"; args[n++]="c++-header";
    args[n++]="Tests/benchmarks/compile/competitive/pch.hpp";
    args[n++]="-o"; args[n++]=output; args[n]=NULL;
    for(i=0;i<n;++i) { if(i) nt_buffer_text(&line," "); nt_quote(&line,args[i]); }
    fprintf(commands,"pch\t%s\tprepare\t0\t%s\n",modes[mode],line.data); fflush(commands); nt_buffer_free(&line);
    cpu=tree_cpu(); r=nt_run(args,root,120000,0); cpu=tree_cpu()-cpu;
    snprintf(log,sizeof log,"%s/%s-prepare.log",out,modes[mode]); nt_write_file(log,r.output,strlen(r.output));
    fprintf(raw,"pch\t%s\tprepare\t0\t0\t%.6f\t%.6f\t%lu\t%llu\n",modes[mode],r.wall_seconds,cpu,(unsigned long)r.exit_code,file_bytes(output)); fflush(raw);
    i=r.started && !r.timed_out && !r.exit_code && file_bytes(output);
    if(!i) fprintf(stderr,"PCH generation failed: %s\n",r.output);
    nt_process_free(&r); return i;
}

static double tree_cpu(void) {
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION a;
    if (!QueryInformationJobObject(accounting, JobObjectBasicAccountingInformation, &a, sizeof a, NULL))
        nt_die("cannot read benchmark CPU accounting", NULL);
    return (a.TotalUserTime.QuadPart + a.TotalKernelTime.QuadPart) / 10000000.0;
}
static unsigned long long file_bytes(const char *path) {
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &a)) return 0;
    return ((unsigned long long)a.nFileSizeHigh << 32) | a.nFileSizeLow;
}
static int sample(const Case *c, int mode, int phase, int run) {
    const char *args[96]; int n = 0, i, ok;
    char bflag[NT_PATH+4], output[NT_PATH], log[NT_PATH], cache[NT_PATH], pch[NT_PATH];
    NtBuffer line = {0}; NtProcessResult r; double cpu;
    snprintf(output, sizeof output, "%s/%s-%s.%s", out, c->name, modes[mode], phase == 2 ? "exe" : "obj");
    snprintf(log, sizeof log, "%s/%s-%s-%s-%d.log", out, c->name, modes[mode], phases[phase], run);
    /* Each packaged CPC needs its own extraction cache: interleaving different
       executable identities must not repeatedly invalidate shared headers. */
    snprintf(cache, sizeof cache, "%s/cache-%s", out, modes[mode]);
    nt_mkdirs(cache);
    if (!SetEnvironmentVariableA("LOCALAPPDATA", cache)) nt_die("set benchmark cache", cache);
#define A(x) args[n++] = (x)
    A(mode == 0 ? cpc : mode == 1 ? fast_cpc : is_minimal(mode) ? minimal : clang);
    if (mode < 2) {
        snprintf(bflag, sizeof bflag, "-B%s", root); A(bflag); A("-O2");
        A("-Iinclude/runtime");
        A("-Iinclude/cprime"); A("-Ithird-party/win32-sdk/include");
        A("-Ithird-party/win32-sdk/include/winapi");
    } else {
        A("-O0"); A("-g0");
        if (c->cpp) { A("-std=c++17"); }
        if (mode >= 3) for(i=0; clang_fast_flags[i]; ++i) A(clang_fast_flags[i]);
        if (mode>=5) {
            snprintf(pch,sizeof pch,"%s/%s.pch",out,modes[mode]); A("-include-pch"); A(pch);
        }
        if (phase == 2) {
            A("-fuse-ld=lld"); A("-Wl,/threads:1,/debug:none,/incremental:no");
        }
    }
    if (c->self) {
        A("-Iinclude/cprime"); A("-Isrc/compiler/frontend"); A("-Isrc/compiler/middleend");
        A("-Isrc/compiler/backend/x64"); A("-I.");
        A("-DCPRIME_TARGET_PE"); A("-DCPRIME_TARGET_X86_64");
        if (mode >= 2) {
            A("-Dopen=_open"); A("-Dread=_read"); A("-Dclose=_close"); A("-Dlseek=_lseek");
            A("-Dunlink=_unlink"); A("-Dfdopen=_fdopen"); A("-Dgetcwd=_getcwd");
            A("-Dstricmp=_stricmp"); A("-Dstrnicmp=_strnicmp"); A("-Dstrlwr=_strlwr");
            A("-Wno-pragma-pack"); A("-Wno-comment"); A("-Wno-ignored-attributes");
            A("-Wno-implicit-function-declaration"); A("-Wno-incompatible-library-redeclaration");
            A("-Wno-deprecated-declarations");
        }
    }
    if (phase == 0) A("-fsyntax-only");
    if (phase == 1) A("-c");
    A(c->source);
    A("-o"); A(output);
    args[n] = NULL;
    for (i=0; i<n; ++i) { if (i) nt_buffer_text(&line, " "); nt_quote(&line, args[i]); }
    fprintf(commands, "%s\t%s\t%s\t%d\t%s\n", c->name, modes[mode], phases[phase], run, line.data);
    fflush(commands); nt_buffer_free(&line);
    if (phase != 0) DeleteFileA(output);
    cpu = tree_cpu(); r = nt_run(args, root, 120000, 0); cpu = tree_cpu() - cpu;
    nt_write_file(log, r.output, strlen(r.output));
    ok = r.started && !r.timed_out && !r.exit_code && (phase == 0 || file_bytes(output));
    fprintf(raw, "%s\t%s\t%s\t%d\t%d\t%.6f\t%.6f\t%lu\t%llu\n",
            c->name, modes[mode], phases[phase], run, run == 0, r.wall_seconds, cpu,
            (unsigned long)(ok ? 0 : r.exit_code ? r.exit_code : 999), phase == 0 ? 0 : file_bytes(output));
    fflush(raw);
    if (!ok) fprintf(stderr, "FAIL %s %s %s: %s\n", c->name, modes[mode], phases[phase], r.output);
    nt_process_free(&r);
    if (ok && phase == 2 && !c->self) {
        const char *execute[] = {output, NULL};
        r = nt_run(execute, root, 10000, 0);
        ok = r.started && !r.timed_out && !r.exit_code;
        i = (int)(c-cases);
        if (ok && expected_stdout[i] && strcmp(expected_stdout[i],r.output)) {
            if (!output_mismatch[i]) fprintf(stderr,"Compiler outputs disagree for %s: first output %s, %s output %s\n",c->name,expected_stdout[i],modes[mode],r.output);
            output_mismatch[i]=1;
        }
        if (ok && !expected_stdout[i]) expected_stdout[i]=nt_strdup(r.output);
        snprintf(log,sizeof log,"%s/%s-%s-run-%d.log",out,c->name,modes[mode],run);
        nt_write_file(log,r.output,strlen(r.output));
        if (!ok) fprintf(stderr, "RUN FAIL %s %s: %lu\n", c->name, modes[mode], (unsigned long)r.exit_code);
        nt_process_free(&r);
    }
    if (ok && phase == 2 && c->self) {
        char produced[NT_PATH];
        const char *execute[24]; int e=0;
        snprintf(produced,sizeof produced,"%s/produced-%s.exe",out,modes[mode]);
        snprintf(bflag,sizeof bflag,"-B%s",root);
        execute[e++]=output; execute[e++]=bflag; execute[e++]="-O2";
        execute[e++]="-Iinclude/runtime"; execute[e++]="-Iinclude/cprime";
        execute[e++]="-Ithird-party/win32-sdk/include";
        execute[e++]="-Ithird-party/win32-sdk/include/winapi";
        execute[e++]="-Isrc/compiler/frontend"; execute[e++]="-Isrc/compiler/middleend";
        execute[e++]="-Isrc/compiler/backend/x64"; execute[e++]="-I.";
        execute[e++]="-DCPRIME_TARGET_PE"; execute[e++]="-DCPRIME_TARGET_X86_64";
        execute[e++]=c->source; execute[e++]="-o"; execute[e++]=produced; execute[e]=NULL;
        for (i=0; i<e; ++i) { if (i) nt_buffer_text(&line," "); nt_quote(&line,execute[i]); }
        fprintf(commands,"%s\t%s\tselfhost\t%d\t%s\n",c->name,modes[mode],run,line.data);
        fflush(commands); nt_buffer_free(&line);
        DeleteFileA(produced); cpu=tree_cpu(); r=nt_run(execute,root,120000,0); cpu=tree_cpu()-cpu;
        ok=r.started && !r.timed_out && !r.exit_code && file_bytes(produced);
        snprintf(log,sizeof log,"%s/%s-selfhost-%d.log",out,modes[mode],run);
        nt_write_file(log,r.output,strlen(r.output));
        fprintf(raw,"%s\t%s\tselfhost\t%d\t%d\t%.6f\t%.6f\t%lu\t%llu\n",
                c->name,modes[mode],run,run==0,r.wall_seconds,cpu,
                (unsigned long)(ok ? 0 : r.exit_code ? r.exit_code : 999),file_bytes(produced));
        fflush(raw);
        if (!ok) fprintf(stderr,"Generated compiler failed %s: %s\n",modes[mode],r.output);
        nt_process_free(&r);
    }
    return ok;
#undef A
}
int main(int argc, char **argv) {
    char scripts[NT_PATH], path[NT_PATH]; DWORD_PTR available, system_mask, chosen;
    int authorized=0, runs=5, i, ci, phase, run, offset, failures=0, pch_ready[MODE_COUNT]={0};
    const char *only = NULL;
    nt_module_directory(scripts, sizeof scripts); strcpy(root, scripts); nt_parent(root);
    nt_join(out, sizeof out, root, "build/clang-competitive/bench");
    nt_join(cpc, sizeof cpc, root, "cpc.exe");
    strcpy(fast_cpc, "C:/Luke/Src/CL/cpc.exe"); strcpy(clang, "C:/Luke/Src/Clang/clang.exe");
    for (i=1; i<argc; ++i) {
        if (!strcmp(argv[i], "-RunExternal")) authorized=1;
        else if (!strcmp(argv[i], "-Root") && i+1<argc) snprintf(root, sizeof root, "%s", argv[++i]);
        else if (!strcmp(argv[i], "-Runs") && i+1<argc) runs=atoi(argv[++i]);
        else if (!strcmp(argv[i], "-Clang") && i+1<argc) snprintf(clang, sizeof clang, "%s", argv[++i]);
        else if (!strcmp(argv[i], "-MinimalClang") && i+1<argc) snprintf(minimal, sizeof minimal, "%s", argv[++i]);
        else if (!strcmp(argv[i], "-Cpc") && i+1<argc) snprintf(cpc, sizeof cpc, "%s", argv[++i]);
        else if (!strcmp(argv[i], "-FastCpc") && i+1<argc) snprintf(fast_cpc, sizeof fast_cpc, "%s", argv[++i]);
        else if (!strcmp(argv[i], "-Out") && i+1<argc) snprintf(out, sizeof out, "%s", argv[++i]);
        else if (!strcmp(argv[i], "-Case") && i+1<argc) only=argv[++i];
        else { fprintf(stderr, "benchmark-clang.exe -RunExternal [-Runs N] [-Root SOURCE_ROOT] [-Clang EXE] [-MinimalClang EXE] [-Cpc EXE] [-FastCpc EXE] [-Out DIR] [-Case NAME]\n"); return 2; }
    }
    if (!authorized || runs<1 || runs>30) { fputs("Requires -RunExternal and 1..30 runs\n", stderr); return 2; }
    if (!GetProcessAffinityMask(GetCurrentProcess(), &available, &system_mask)) nt_die("read affinity", NULL);
    chosen = available & (0 - available);
    if (!SetProcessAffinityMask(GetCurrentProcess(), chosen)) nt_die("set one-CPU affinity", NULL);
    accounting = CreateJobObjectA(NULL, NULL);
    if (!accounting || !AssignProcessToJobObject(accounting, GetCurrentProcess())) nt_die("create CPU accounting job", NULL);
    nt_mkdirs(out);
    nt_join(path, sizeof path, out, "raw.tsv"); raw = fopen(path, "wb");
    nt_join(path, sizeof path, out, "commands.tsv"); commands = fopen(path, "wb");
    if (!raw || !commands) nt_die("open benchmark results", out);
    fprintf(raw, "case\tcompiler\tphase\trun\twarmup\twall_s\ttree_cpu_s\texit\tbytes\n");
    printf("Serial benchmark, affinity mask %llu, %d measured runs + one warmup\n", (unsigned long long)chosen, runs); fflush(stdout);
    for (ci=0; ci<(int)(sizeof cases/sizeof cases[0]); ++ci) {
        if (only && strcmp(only, cases[ci].name)) continue;
        for (phase=0; phase<3; ++phase) {
            int failed[MODE_COUNT] = {0};
            for (run=0; run<=runs; ++run) for (offset=0; offset<MODE_COUNT; ++offset) {
                int mode=(offset+run)%MODE_COUNT;
                if(is_minimal(mode) && !*minimal) continue;
                if(mode>=5 && !cases[ci].stl) continue;
                if(mode>=5 && !pch_ready[mode]) {
                    pch_ready[mode]=prepare_pch(mode) ? 1 : -1;
                    if(pch_ready[mode]<0) failures++;
                }
                if(mode>=5 && pch_ready[mode]<0) continue;
                /* CPC silently ignores -fsyntax-only; do not mislabel a full
                   compile/link as a front-end measurement. */
                if (phase == 0 && mode < 2) continue;
                if (!failed[mode] && !sample(&cases[ci], mode, phase, run)) { failed[mode]=1; failures++; }
            }
            printf("Completed %s %s\n", cases[ci].name, phases[phase]); fflush(stdout);
        }
    }
    fclose(raw); fclose(commands); CloseHandle(accounting);
    for (ci=0; ci<(int)(sizeof cases/sizeof cases[0]); ++ci) if (output_mismatch[ci]) {
        fprintf(stderr,"INVALID semantic-equivalence comparison: %s\n",cases[ci].name); failures++;
    }
    printf("Results: %s; failures: %d\n", out, failures);
    return failures ? 1 : 0;
}
