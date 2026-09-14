#include "common/native_tool.h"

typedef struct ToolSpec {
    const char *source;
    const char *output;
    const char *library;
} ToolSpec;

static const ToolSpec tools[] = {
    {"scripts\\build.c", "scripts\\build.exe", NULL},
    {"scripts\\project.c", "scripts\\project.exe", "-ladvapi32"},
    {"scripts\\project-clang.c", "scripts\\project-clang.exe", "-ladvapi32"},
    {"scripts\\performance.c", "scripts\\performance.exe", NULL},
    {"scripts\\performance-tcc.c", "scripts\\performance-tcc.exe", NULL},
    {"scripts\\profile.c", "scripts\\profile.exe", "-lwinmm"},
    {"scripts\\deploy.c", "scripts\\deploy.exe", NULL},
    {"scripts\\maintenance.c", "scripts\\maintenance.exe", NULL},
    {"scripts\\build-clang.c", "scripts\\build-clang.exe", NULL},
    {"scripts\\seed-tcc.c", "scripts\\seed-tcc.exe", NULL},
    {"Tests\\tools\\test_runner.c", "Tests\\test.exe", NULL},
    {"Tests\\tools\\test_msvc.c", "Tests\\test-msvc.exe", NULL}
};

static void replace_file(const char *from, const char *to) {
    if (!MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        nt_die("cannot publish workflow executable", to);
}

int main(int argc, char **argv) {
    char scripts[NT_PATH], root[NT_PATH], cpc[NT_PATH];
    int i, failures = 0;
    (void)argc; (void)argv;
    nt_module_directory(scripts, sizeof scripts);
    strcpy(root, scripts); nt_parent(root);
    nt_join(cpc, sizeof cpc, root, "cpc.exe");
    if (!nt_exists(cpc)) nt_die("root compiler missing", cpc);
    for (i = 0; i < (int)(sizeof tools / sizeof tools[0]); ++i) {
        char source[NT_PATH], output[NT_PATH], staged[NT_PATH];
        const char *command[8];
        NtProcessResult result;
        int n = 0;
        nt_join(source, sizeof source, root, tools[i].source);
        nt_join(output, sizeof output, root, tools[i].output);
        snprintf(staged, sizeof staged, "%s.new.exe", output);
        if (!nt_exists(source)) { fprintf(stderr, "missing tool source: %s\n", source); failures++; continue; }
        command[n++] = cpc; command[n++] = "-O2"; command[n++] = "-o";
        command[n++] = staged; command[n++] = source;
        if (tools[i].library) command[n++] = tools[i].library;
        command[n] = NULL;
        printf("[%d/%d] %s\n", i + 1, (int)(sizeof tools / sizeof tools[0]), tools[i].output);
        result = nt_run(command, root, 300000, 1);
        if (!result.started || result.timed_out || result.exit_code || !nt_exists(staged)) {
            fprintf(stderr, "failed to build %s\n", tools[i].output);
            DeleteFileA(staged); failures++;
        } else replace_file(staged, output);
        nt_process_free(&result);
        if (failures) break;
    }
    if (failures) return 1;
    puts("Native workflow executables are current.");
    return 0;
}
