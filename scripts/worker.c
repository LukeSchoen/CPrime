#include "common/native_tool.h"
#include <time.h>

static const char *env_or(const char *name, const char *fallback) {
    const char *value = getenv(name); return value && *value ? value : fallback;
}

static int run_simple(const char *root, const char *const *command, unsigned timeout, int echo) {
    NtProcessResult result = nt_run(command, root, timeout, echo);
    int code = !result.started || result.timed_out ? 1 : (int)result.exit_code;
    nt_process_free(&result); return code;
}

static int git_commit(const char *root, int cycle) {
    char git[NT_PATH];
    const char *add[] = {git, "add", "-A", NULL};
    const char *diff[] = {git, "diff", "--cached", "--quiet", NULL};
    char message[128];
    const char *commit[] = {git, "commit", "-m", message, NULL};
    DWORD length = SearchPathA(NULL, "git.exe", NULL, sizeof git, git, NULL);
    if (!length || length >= sizeof git) return 1;
    if (run_simple(root, add, 30000, 0)) return 1;
    if (!run_simple(root, diff, 30000, 0)) return 0;
    snprintf(message, sizeof message, "native worker cycle %d", cycle);
    return run_simple(root, commit, 120000, 1);
}

static int read_last_cycle(const char *root, const char *status, const char *log) {
    const char *command[] = {status, "--root", root, "--log", log, "--last-cycle", NULL};
    NtProcessResult result = nt_run(command, root, 30000, 0);
    int cycle = result.started && !result.timed_out && !result.exit_code ? atoi(result.output) : 0;
    nt_process_free(&result); return cycle;
}

int main(int argc, char **argv) {
    char scripts[NT_PATH], root[NT_PATH], cpc[NT_PATH], status[NT_PATH], status_src[NT_PATH];
    char cycle_log[NT_PATH], result_log[NT_PATH], cycle_text[32], model_arg[] = "model_reasoning_effort=medium";
    const char *done = env_or("DONE", "done.x"), *codex = env_or("CODEX_EXE", "codex.exe");
    const char *progress = env_or("PROGRESS_LOG", "Tests\\progress\\log.tsv");
    const char *corpus = env_or("CORPUS", "Tests\\pedantic\\gcc\\corpus.json");
    const char *first = env_or("FIRST_PARTY", "Tests\\progress\\first-party-failures.txt");
    const char *prompt = env_or("TASK_PROMPT", "Complete task.md and Performance/task.md following AGENTS.md. Use scripts/build.exe for validated publication and native test/workflow executables. Continue useful authorized work until acceptance is recorded.");
    int max_cycles = atoi(env_or("MAX_CYCLES", "0")), failure_limit = atoi(env_or("FAIL_EXIT_LIMIT", "3"));
    int cycle, session = 0, failures = 0, i;
    nt_module_directory(scripts, sizeof scripts); strcpy(root, scripts); nt_parent(root);
    for (i = 1; i < argc; ++i) {
        if (!_stricmp(argv[i], "-Help") || !_stricmp(argv[i], "--help")) {
            puts("worker.exe\nEnvironment: DONE CODEX_EXE TASK_PROMPT MAX_CYCLES FAIL_EXIT_LIMIT PROGRESS_LOG FIRST_PARTY CORPUS\nRuns one foreground Codex cycle at a time; no fixed sleep or compiler concurrency."); return 0;
        }
        fprintf(stderr, "unknown worker option: %s\n", argv[i]); return 2;
    }
    nt_join(cpc, sizeof cpc, root, "cpc.exe"); nt_join(status, sizeof status, root, "build\\worker-status.exe");
    nt_join(status_src, sizeof status_src, root, "src\\tools\\worker_status.c");
    { const char *build[] = {cpc, "-O2", "-o", status, status_src, NULL};
      if (run_simple(root, build, 120000, 0) && !nt_exists(status)) nt_die("cannot build worker status tool", status_src); }
    cycle = read_last_cycle(root, status, progress);
    printf("CPrime native worker starting in %s at cycle %d\n", root, cycle);
    for (;;) {
        char done_path[NT_PATH];
        nt_join(done_path, sizeof done_path, root, done);
        if (nt_exists(done_path)) { printf("stop marker present after %d session cycle(s)\n", session); return 0; }
        if (max_cycles > 0 && session >= max_cycles) return 0;
        if (git_commit(root, cycle)) fprintf(stderr, "warning: pre-cycle commit failed\n");
        cycle++; session++; snprintf(cycle_text, sizeof cycle_text, "%d", cycle);
        snprintf(cycle_log, sizeof cycle_log, "%s\\build\\worker-cycle-%d.log", root, cycle);
        snprintf(result_log, sizeof result_log, "%s\\build\\worker-cycle-%d-result.txt", root, cycle);
        { const char *command[] = {codex, "exec", "--model", "gpt-5.6-terra", "-c", model_arg,
              "-C", root, "--sandbox", "danger-full-access", "-c", "approval_policy=never",
              "--color", "never", "--output-last-message", result_log, prompt, NULL};
          NtProcessResult result = nt_run(command, root, 24u * 60u * 60u * 1000u, 0);
          if (result.output) nt_write_file(cycle_log, result.output, strlen(result.output));
          if (!result.started || result.timed_out || result.exit_code) failures++; else failures = 0;
          printf("cycle %d exit %lu; output %s\n", cycle, (unsigned long)result.exit_code, cycle_log);
          nt_process_free(&result); }
        { const char *report[] = {status, "--brief", "--root", root, "--log", progress,
              "--corpus", corpus, "--first-party", first, "--append", "--cycle", cycle_text,
              "--event", "cycle", "--head", "-", NULL};
          run_simple(root, report, 30000, 1); }
        if (failures >= failure_limit) return 1;
    }
}
