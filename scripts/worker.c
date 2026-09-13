#include "common/native_tool.h"
#include <ctype.h>
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

static int read_session(const char *path, char *session_id, size_t capacity) {
    size_t size = 0, length;
    unsigned char *data = nt_read_file(path, &size);
    if (!data) return 0;
    while (size && isspace(data[size - 1])) size--;
    length = size < capacity - 1 ? size : capacity - 1;
    memcpy(session_id, data, length);
    session_id[length] = 0;
    free(data);
    return length != 0;
}

static int extract_session(const char *output, char *session_id, size_t capacity) {
    const char *value = output ? strstr(output, "session id:") : NULL;
    size_t length = 0;
    if (!value) return 0;
    value += strlen("session id:");
    while (*value && isspace((unsigned char)*value)) value++;
    while (value[length] && !isspace((unsigned char)value[length]) && length + 1 < capacity)
        length++;
    if (!length) return 0;
    memcpy(session_id, value, length);
    session_id[length] = 0;
    return 1;
}

static int recover_session(const char *root, int cycle, char *session_id, size_t capacity) {
    char path[NT_PATH];
    unsigned char *output;
    size_t size = 0;
    int found;
    if (cycle <= 0) return 0;
    snprintf(path, sizeof path, "%s\\build\\worker-cycle-%d.log", root, cycle);
    output = nt_read_file(path, &size);
    if (!output) return 0;
    found = extract_session((const char *)output, session_id, capacity);
    free(output);
    return found;
}

int main(int argc, char **argv) {
    char scripts[NT_PATH], root[NT_PATH], cpc[NT_PATH], status[NT_PATH], status_src[NT_PATH];
    char cycle_log[NT_PATH], result_log[NT_PATH], session_path[NT_PATH], cycle_text[32];
    char session_id[128], model_arg[] = "model_reasoning_effort=medium";
    const char *done = env_or("DONE", "done.x"), *codex = env_or("CODEX_EXE", "codex.exe");
    const char *progress = env_or("PROGRESS_LOG", "Tests\\progress\\log.tsv");
    const char *corpus = env_or("CORPUS", "Tests\\pedantic\\gcc\\corpus.json");
    const char *first = env_or("FIRST_PARTY", "Tests\\progress\\first-party-failures.txt");
    const char *prompt = env_or("TASK_PROMPT", "Complete task.md and Performance/task.md following AGENTS.md. Move aggressively through the remaining correctness work. Batch-reassess all retained failures in the chosen subsystem, group them by shared mechanism, and attack the largest coherent root-cause cluster rather than pecking at one row. During compiler edit loops use scripts/build.exe -NoPack -NoValidate -NoPublish, then exercise the whole affected cluster plus positive and negative controls in one serial native-runner invocation. When a fix exposes another blocker in that cluster, continue through it in the same turn. Publish and run broad gates once the cluster is coherent. Continue authorized work until acceptance; an unauthorized external gate is not a reason to stop.");
    const char *continue_prompt = env_or("CONTINUE_PROMPT", "Resume the established investigation and drive it hard. Do not rediscover repository state, re-explain known blockers, or end after one row. Batch-reassess the chosen subsystem, select the largest shared-mechanism cluster, and iterate until that cluster is repaired or evidence proves its rows independent. For each diagnostic candidate use scripts/build.exe -NoPack -NoValidate -NoPublish, then run the affected retained rows, the focused first-party regressions, and adversarial positive/negative controls together in one serial native-runner batch. Follow newly exposed blockers immediately within this turn. Only after the cluster passes, run one validated publishing build and the relevant broader gate, retire every proven repaired row, update the durable handoff, and move directly to the next cluster while authorized work remains.");
    const char *session_file = env_or("SESSION_FILE", "build\\worker-session.txt");
    int max_cycles = atoi(env_or("MAX_CYCLES", "0")), failure_limit = atoi(env_or("FAIL_EXIT_LIMIT", "3"));
    int cycle, session = 0, failures = 0, i, has_session;
    nt_module_directory(scripts, sizeof scripts); strcpy(root, scripts); nt_parent(root);
    for (i = 1; i < argc; ++i) {
        if (!_stricmp(argv[i], "-Help") || !_stricmp(argv[i], "--help")) {
            puts("worker.exe\nEnvironment: DONE CODEX_EXE TASK_PROMPT CONTINUE_PROMPT SESSION_FILE MAX_CYCLES FAIL_EXIT_LIMIT PROGRESS_LOG FIRST_PARTY CORPUS\nRuns one foreground Codex session continuously across cycles; no fixed sleep or compiler concurrency."); return 0;
        }
        fprintf(stderr, "unknown worker option: %s\n", argv[i]); return 2;
    }
    nt_join(cpc, sizeof cpc, root, "cpc.exe"); nt_join(status, sizeof status, root, "build\\worker-status.exe");
    nt_join(status_src, sizeof status_src, root, "src\\tools\\worker_status.c");
    nt_join(session_path, sizeof session_path, root, session_file);
    { const char *build[] = {cpc, "-O2", "-o", status, status_src, NULL};
      if (run_simple(root, build, 120000, 0) && !nt_exists(status)) nt_die("cannot build worker status tool", status_src); }
    cycle = read_last_cycle(root, status, progress);
    has_session = read_session(session_path, session_id, sizeof session_id);
    if (!has_session && recover_session(root, cycle, session_id, sizeof session_id)) {
        nt_write_file(session_path, session_id, strlen(session_id));
        has_session = 1;
    }
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
        { const char *command[24];
          NtProcessResult result;
          int n = 0;
          command[n++] = codex; command[n++] = "exec"; command[n++] = "--model";
          command[n++] = "gpt-5.6-terra"; command[n++] = "-c"; command[n++] = model_arg;
          command[n++] = "-C"; command[n++] = root; command[n++] = "--sandbox";
          command[n++] = "danger-full-access"; command[n++] = "-c";
          command[n++] = "approval_policy=never"; command[n++] = "--color";
          command[n++] = "never"; command[n++] = "--output-last-message";
          command[n++] = result_log;
          if (has_session) {
              command[n++] = "resume"; command[n++] = session_id; command[n++] = continue_prompt;
          } else {
              command[n++] = prompt;
          }
          command[n] = NULL;
          result = nt_run(command, root, 24u * 60u * 60u * 1000u, 0);
          if (!result.started) {
              char diagnostic[NT_PATH + 128];
              snprintf(diagnostic, sizeof diagnostic,
                       "worker: could not start %s (Windows error %lu)\n",
                       codex, (unsigned long)result.system_error);
              nt_write_file(cycle_log, diagnostic, strlen(diagnostic));
              fputs(diagnostic, stderr);
          } else if (result.output) {
              nt_write_file(cycle_log, result.output, strlen(result.output));
          }
          if (result.started && !result.timed_out && !result.exit_code && !has_session &&
              extract_session(result.output, session_id, sizeof session_id)) {
              nt_write_file(session_path, session_id, strlen(session_id));
              has_session = 1;
          } else if (has_session && (!result.started || result.timed_out || result.exit_code)) {
              /* A stale or unusable session must not trap all following cycles. */
              DeleteFileA(session_path);
              has_session = 0;
          }
          if (!result.started || result.timed_out || result.exit_code) failures++; else failures = 0;
          if (result.started)
              printf("cycle %d exit %lu; output %s\n", cycle, (unsigned long)result.exit_code, cycle_log);
          nt_process_free(&result); }
        { const char *report[] = {status, "--brief", "--root", root, "--log", progress,
              "--corpus", corpus, "--first-party", first, "--append", "--cycle", cycle_text,
              "--event", "cycle", "--head", "-", NULL};
          run_simple(root, report, 30000, 1); }
        if (failures >= failure_limit) return 1;
    }
}
