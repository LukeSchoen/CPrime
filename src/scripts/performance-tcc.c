/* Explicitly named external-compiler comparison entry point. */
#define main performance_compare_main
#include "../tools/perf_compare.c"
#undef main

int main(int argc, char **argv) {
    int i, out = 1, authorized = 0;
    for (i = 1; i < argc; ++i) {
        if (!_stricmp(argv[i], "-RunExternal")) authorized = 1;
        else argv[out++] = argv[i];
    }
    argv[out] = NULL;
    if (!authorized) {
        fprintf(stderr, "TCC execution requires the explicit -RunExternal switch.\n");
        return 2;
    }
    return performance_compare_main(out, argv);
}
