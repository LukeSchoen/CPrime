#include "common/native_tool.h"
#include <ctype.h>

static int forbidden_extension(const char *path) {
    const char *dot = strrchr(path, '.');
    return dot && (!_stricmp(dot, ".bat") || !_stricmp(dot, ".cmd") ||
                   !_stricmp(dot, ".ps1") || !_stricmp(dot, ".py"));
}

static int is_preserved_third_party(const char *path) {
    return !_stricmp(path, "src/third-party/tcc/win32/build-tcc.bat") ||
           !_stricmp(path, "src/third-party/tcc/tests/test-win32.bat");
}

/* The user owns these three agent-loop control surfaces and AGENTS.md exempts
   them from the no-first-party-script rule. */
static int is_worker_control_surface(const char *path) {
    return !_stricmp(path, "Capability/worker.cmd") ||
           !_stricmp(path, "Compatibility/worker.cmd") ||
           !_stricmp(path, "Cost/worker.cmd");
}

static int check_sources(const char *root) {
    char git[NT_PATH];
    const char *command[] = {git, "ls-files", "--cached", "--others", "--exclude-standard", NULL};
    DWORD git_length = SearchPathA(NULL, "git.exe", NULL, sizeof git, git, NULL);
    if (!git_length || git_length >= sizeof git) nt_die("git.exe not found on PATH", "");
    NtProcessResult result = nt_run(command, root, 30000, 0);
    char *line, *next;
    int failures = 0;
    if (!result.started || result.timed_out || result.exit_code) {
        fprintf(stderr, "git ls-files failed (started=%d timeout=%d exit=%lu): %s\n",
                result.started, result.timed_out, (unsigned long)result.exit_code,
                result.output ? result.output : "");
        nt_process_free(&result); return 1;
    }
    for (line = result.output; line && *line; line = next) {
        next = strchr(line, '\n');
        if (next) *next++ = 0;
        while (*line && (line[strlen(line) - 1] == '\r' || line[strlen(line) - 1] == '\n'))
            line[strlen(line) - 1] = 0;
        {
            char full[NT_PATH]; nt_join(full, sizeof full, root, line);
            if (!nt_exists(full)) continue;
        }
        if (forbidden_extension(line) && !is_preserved_third_party(line) &&
            !is_worker_control_surface(line)) {
            fprintf(stderr, "forbidden first-party script: %s\n", line);
            failures++;
        }
    }
    nt_process_free(&result);
    if (!failures)
        puts("PASS native source policy (only the two preserved TCC batch files "
             "and the three worker control surfaces remain)");
    return failures ? 1 : 0;
}

static int clean_logs(const char *root) {
    WIN32_FIND_DATAA entry;
    HANDLE find;
    char pattern[NT_PATH], path[NT_PATH];
    unsigned removed = 0;
    nt_join(pattern, sizeof pattern, root, "*.log");
    find = FindFirstFileA(pattern, &entry);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if (!(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                nt_join(path, sizeof path, root, entry.cFileName);
                if (DeleteFileA(path)) removed++;
            }
        } while (FindNextFileA(find, &entry));
        FindClose(find);
    }
    printf("Removed %u root diagnostic log(s); generated build evidence was preserved.\n", removed);
    return 0;
}

static void usage(void) {
    puts("maintenance.exe -CheckSources\n"
         "maintenance.exe -CleanLogs\n"
         "Native repository policy and low-frequency maintenance.");
}

int main(int argc, char **argv) {
    char scripts[NT_PATH], root[NT_PATH];
    nt_module_directory(scripts, sizeof scripts); nt_tree_root(root, sizeof root);
    if (argc != 2 || !_stricmp(argv[1], "-Help") || !_stricmp(argv[1], "--help")) {
        usage(); return argc == 2 ? 0 : 2;
    }
    if (!_stricmp(argv[1], "-CheckSources")) return check_sources(root);
    if (!_stricmp(argv[1], "-CleanLogs")) return clean_logs(root);
    usage(); return 2;
}
