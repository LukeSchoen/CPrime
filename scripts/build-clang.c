#include "common/native_tool.h"

static void usage(void) {
    puts("build-clang.exe -RunExternal [-ClangPath PATH] [-OutDir DIR] [-O 0|1|2|3|s|z] [-SystemCRT] [-Map]\n"
         "Clang is never invoked without the explicit -RunExternal authorization switch.");
}

int main(int argc, char **argv) {
    char scripts[NT_PATH], root[NT_PATH], clang[NT_PATH], out[NT_PATH], exe[NT_PATH];
    const char *opt = "3", *args[64];
    int authorized = 0, system_crt = 0, map = 0, i, n = 0;
    NtProcessResult result;
    nt_module_directory(scripts, sizeof scripts); strcpy(root, scripts); nt_parent(root);
    nt_join(clang, sizeof clang, root, "third-party\\clang\\bin\\clang.exe");
    nt_join(out, sizeof out, root, "build\\clang");
    for (i = 1; i < argc; ++i) {
        if (!_stricmp(argv[i], "-RunExternal")) authorized = 1;
        else if (!_stricmp(argv[i], "-SystemCRT")) system_crt = 1;
        else if (!_stricmp(argv[i], "-Map")) map = 1;
        else if (!_stricmp(argv[i], "-ClangPath") && i + 1 < argc) strcpy(clang, argv[++i]);
        else if (!_stricmp(argv[i], "-OutDir") && i + 1 < argc) strcpy(out, argv[++i]);
        else if (!_stricmp(argv[i], "-O") && i + 1 < argc) opt = argv[++i];
        else if (!_stricmp(argv[i], "-Help") || !_stricmp(argv[i], "--help")) { usage(); return 0; }
        else { usage(); return 2; }
    }
    if (!authorized) { fprintf(stderr, "External Clang execution requires -RunExternal.\n"); return 2; }
    if (!nt_exists(clang)) nt_die("Clang executable not found", clang);
    nt_mkdirs(out); nt_join(exe, sizeof exe, out, "cpc-clang.exe");
#define ADD(x) args[n++] = (x)
    ADD(clang); ADD("-I"); { static char p[NT_PATH]; nt_join(p,sizeof p,root,"include\\cprime"); ADD(p); }
    ADD("-I"); { static char p[NT_PATH]; nt_join(p,sizeof p,root,"src\\compiler\\frontend"); ADD(p); }
    ADD("-I"); { static char p[NT_PATH]; nt_join(p,sizeof p,root,"src\\compiler\\middleend"); ADD(p); }
    ADD("-I"); { static char p[NT_PATH]; nt_join(p,sizeof p,root,"src\\compiler\\backend\\x64"); ADD(p); }
    ADD("-I"); ADD(root); ADD("-DCPRIME_TARGET_PE"); ADD("-DCPRIME_TARGET_X86_64");
    ADD("-Dopen=_open"); ADD("-Dread=_read"); ADD("-Dclose=_close"); ADD("-Dlseek=_lseek");
    ADD("-Dunlink=_unlink"); ADD("-Dfdopen=_fdopen"); ADD("-Dgetcwd=_getcwd"); ADD("-Dstricmp=_stricmp");
    ADD("-Dstrnicmp=_strnicmp"); ADD("-Dstrlwr=_strlwr"); ADD("-Wno-pragma-pack"); ADD("-Wno-comment");
    ADD("-Wno-ignored-attributes"); ADD("-Wno-implicit-function-declaration");
    ADD("-Wno-incompatible-library-redeclaration"); ADD("-Wno-deprecated-declarations");
    { static char o[8]; snprintf(o,sizeof o,"-O%s",opt); ADD(o); } ADD("-g0");
    ADD("-Wl,/DEBUG:NONE,/INCREMENTAL:NO");
    if (system_crt) { ADD("-D_DLL"); ADD("-D_MT"); ADD("-Xclang"); ADD("--dependent-lib=msvcrt");
        ADD("-Wl,/NODEFAULTLIB:libcmt,/NODEFAULTLIB:libucrt,/NODEFAULTLIB:vcruntime,/NODEFAULTLIB:libvcruntime,/DEFAULTLIB:ucrt");
        ADD("-Xlinker"); ADD("libvcruntime.lib"); }
    if (map) { static char m[NT_PATH + 16]; snprintf(m,sizeof m,"-Wl,/MAP:%s\\cpc-clang.map",out); ADD(m); }
    { static char source[NT_PATH]; nt_join(source,sizeof source,root,"src\\compiler\\driver\\cprime.c"); ADD(source); }
    ADD("-o"); ADD(exe); args[n] = NULL;
    result = nt_run(args, root, 600000, 1);
    if (!result.started || result.timed_out || result.exit_code || !nt_exists(exe)) {
        fprintf(stderr, "Clang CPC build failed.\n"); nt_process_free(&result); return 1;
    }
    printf("Clang compile/link: %.3fs; wrote %s\n", result.wall_seconds, exe);
    nt_process_free(&result); return 0;
}
