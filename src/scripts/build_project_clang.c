#define BUILD_PROJECT_EXTERNAL 1
#define wmain project_clang_wmain
#include "../tools/build_project.c"
#undef wmain

int main(int argc, char **argv) {
    int i, out = 1, authorized = 0;
    for (i = 1; i < argc; ++i) {
        if (!_stricmp(argv[i], "-RunExternal")) authorized = 1;
        else argv[out++] = argv[i];
    }
    argv[out] = NULL;
    if (!authorized) {
        fprintf(stderr, "External Clang project compilation requires -RunExternal.\n");
        return 2;
    }
    return project_main(out, argv);
}
