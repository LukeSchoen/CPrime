#include "../../scripts/common/native_tool.h"
#include <ctype.h>

typedef struct TestMeta {
    int expected_exit;
    int compile_fail;
    int link_fail;
    int compile_only;
    char expected_stdout[1024];
    char compile_args[4096];
    char sources[4096];
    char manifest_source[NT_PATH];
} TestMeta;

typedef struct TestCase {
    char path[NT_PATH];
    char name[512];
    int fail_directory;
} TestCase;

typedef struct TestList {
    TestCase *items;
    int count;
    int capacity;
} TestList;

static char root[NT_PATH], tests_root[NT_PATH];

static int compare_tests(const void *left, const void *right) {
    const TestCase *a = left, *b = right;
    int kind = a->fail_directory - b->fail_directory;
    return kind ? kind : _stricmp(a->name, b->name);
}

static void list_add(TestList *list, const char *path, const char *name, int fail_directory) {
    TestCase *item;
    if (list->count == list->capacity) {
        int capacity = list->capacity ? list->capacity * 2 : 64;
        TestCase *next = realloc(list->items, capacity * sizeof *next);
        if (!next) nt_die("out of memory", "");
        list->items = next;
        list->capacity = capacity;
    }
    item = &list->items[list->count++];
    if (strlen(path) >= sizeof item->path || strlen(name) >= sizeof item->name)
        nt_die("test path too long", path);
    strcpy(item->path, path);
    strcpy(item->name, name);
    item->fail_directory = fail_directory;
}

static int test_extension(const char *name) {
    const char *dot = strrchr(name, '.');
    return !_strnicmp(name, "test_", 5) && dot &&
           (!_stricmp(dot, ".c") || !_stricmp(dot, ".cpp"));
}

static void discover_directory(TestList *list, const char *path, int fail_directory) {
    WIN32_FIND_DATAA entry;
    HANDLE find;
    char pattern[NT_PATH], file[NT_PATH];
    if (!nt_is_directory(path)) return;
    nt_join(pattern, sizeof pattern, path, "*");
    find = FindFirstFileA(pattern, &entry);
    if (find == INVALID_HANDLE_VALUE) nt_die("cannot enumerate tests", path);
    do {
        if (!(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && test_extension(entry.cFileName)) {
            nt_join(file, sizeof file, path, entry.cFileName);
            list_add(list, file, entry.cFileName, fail_directory);
        }
    } while (FindNextFileA(find, &entry));
    FindClose(find);
}

static void trim(char *text) {
    char *start = text, *end;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != text) memmove(text, start, strlen(start) + 1);
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = 0;
}

static void parse_meta(const char *path, TestMeta *meta) {
    FILE *file = fopen(path, "rb");
    char line[8192];
    int count = 0;
    memset(meta, 0, sizeof *meta);
    if (!file) nt_die("cannot read test", path);
    while (count++ < 12 && fgets(line, sizeof line, file)) {
        char key[64], *colon, *value;
        if (strncmp(line, "//", 2)) continue;
        colon = strchr(line, ':');
        if (!colon) continue;
        *colon = 0;
        strncpy(key, line + 2, sizeof key - 1);
        key[sizeof key - 1] = 0;
        trim(key);
        value = colon + 1;
        trim(value);
        if (!strcmp(key, "EXPECT_EXIT")) meta->expected_exit = atoi(value);
        else if (!strcmp(key, "EXPECT_COMPILE_FAIL")) meta->compile_fail = atoi(value) != 0;
        else if (!strcmp(key, "EXPECT_LINK_FAIL")) meta->link_fail = atoi(value) != 0;
        else if (!strcmp(key, "EXPECT_COMPILE_ONLY")) meta->compile_only = atoi(value) != 0;
        else if (!strcmp(key, "EXPECT_STDOUT")) {
            strncpy(meta->expected_stdout, value, sizeof meta->expected_stdout - 1);
        } else if (!strcmp(key, "EXPECT_COMPILE_ARGS")) {
            strncpy(meta->compile_args, value, sizeof meta->compile_args - 1);
        } else if (!strcmp(key, "EXPECT_SOURCES")) {
            strncpy(meta->sources, value, sizeof meta->sources - 1);
        } else if (!strcmp(key, "EXPECT_MANIFEST_SOURCE")) {
            strncpy(meta->manifest_source, value, sizeof meta->manifest_source - 1);
        }
    }
    fclose(file);
    if (meta->link_fail && (meta->compile_fail || meta->compile_only))
        nt_die("link-failure expectation conflicts with compile-only expectations", path);
}

static int split_arguments(char *text, char **arguments, int capacity) {
    char *read = text, *write = text;
    int count = 0, quoted = 0, started = 0;
    while (*read) {
        if (!started) {
            while (isspace((unsigned char)*read)) read++;
            if (!*read) break;
            if (count == capacity) nt_die("too many compiler arguments", text);
            arguments[count++] = write;
            started = 1;
        }
        if (*read == '"') { quoted = !quoted; read++; continue; }
        if (*read == '\\' && (read[1] == '\\' || read[1] == '"')) read++;
        if (!quoted && isspace((unsigned char)*read)) {
            *write++ = 0;
            read++;
            started = 0;
            continue;
        }
        *write++ = *read++;
    }
    if (quoted) nt_die("unterminated quote in EXPECT_COMPILE_ARGS", text);
    if (started) *write = 0;
    return count;
}

static void response_argument(NtBuffer *response, const char *argument) {
    const char *p;
    nt_buffer_text(response, "\"");
    for (p = argument; *p; ++p) {
        if (*p == '\\' || *p == '"') nt_buffer_text(response, "\\");
        nt_buffer_append(response, p, 1);
    }
    nt_buffer_text(response, "\"\n");
}

static void normalize_output(char *text) {
    char *read = text, *write = text;
    while (*read) {
        if (*read == '\r') { read++; if (*read == '\n') continue; *write++ = '\n'; }
        else *write++ = *read++;
    }
    while (write > text && write[-1] == '\n') --write;
    *write = 0;
}

static int selected(const char *name, char **selection, int count) {
    int i;
    if (!count) return 1;
    for (i = 0; i < count; ++i) if (!_stricmp(name, selection[i])) return 1;
    return 0;
}

static int tier_contains(const char *relative) {
    char path[NT_PATH], needle[NT_PATH + 8];
    size_t size;
    static unsigned char *data;
    if (!data) {
        nt_join(path, sizeof path, tests_root, "tiers.json");
        data = nt_read_file(path, &size);
    }
    if (!data) return 0;
    snprintf(needle, sizeof needle, "\"%s\"", relative);
    {
        int found = strstr((char *)data, needle) != NULL;
        return found;
    }
}

typedef struct RegressionCase {
    char path[NT_PATH];
    char name[512];
    char output[NT_PATH];
    TestMeta meta;
} RegressionCase;

static void job_argument(NtBuffer *batch, const char *argument);
static int append_additional_job_sources(NtBuffer *batch, const char *test_path,
                                         const char *json);
static int find_batch_exit(const char *output, int job, DWORD *code);

static int append_additional_sources(NtBuffer *response, const TestCase *test,
                                     const char *json) {
    char directory[NT_PATH], value[NT_PATH], full[NT_PATH];
    const char *p = json;
    int count = 0;
    if (!*json) return 0;
    if (*p++ != '[') nt_die("EXPECT_SOURCES must be a JSON array", test->path);
    strcpy(directory, test->path);
    nt_parent(directory);
    while (*p) {
        size_t used = 0;
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (*p == ']') break;
        if (*p++ != '"') nt_die("invalid EXPECT_SOURCES", test->path);
        while (*p && *p != '"') {
            if (*p == '\\' && p[1]) p++;
            if (used + 1 >= sizeof value) nt_die("additional source path too long", test->path);
            value[used++] = *p++;
        }
        if (*p++ != '"') nt_die("invalid EXPECT_SOURCES", test->path);
        value[used] = 0;
        if (strstr(value, "..") || strchr(value, ':') || value[0] == '/' || value[0] == '\\')
            nt_die("unsafe EXPECT_SOURCES path", value);
        nt_join(full, sizeof full, directory, value);
        if (!nt_exists(full)) nt_die("additional source missing", full);
        response_argument(response, full);
        count++;
    }
    return count;
}

static int run_link_failure(const TestCase *test, const TestMeta *meta,
                            const char *compiler, const char *runtime,
                            const char *work, unsigned timeout_ms) {
    NtBuffer source_list = {0}, evidence = {0};
    char flags[4096], runtime_arg[NT_PATH + 2], executable[NT_PATH], log[NT_PATH];
    char *sources[128], *extra[128], (*objects)[NT_PATH];
    const char *command[264];
    int source_count, extra_count, stage, i, n, ok = 1;
    response_argument(&source_list, test->path);
    append_additional_sources(&source_list, test, meta->sources);
    source_count = split_arguments((char *)source_list.data, sources, 128);
    strcpy(flags, meta->compile_args);
    extra_count = split_arguments(flags, extra, 128);
    objects = nt_alloc(source_count * sizeof *objects);
    snprintf(runtime_arg, sizeof runtime_arg, "-B%s", runtime ? runtime : "");
    snprintf(executable, sizeof executable, "%s\\%s-link.exe", work, test->name);
    snprintf(log, sizeof log, "%s\\%s-link.log", work, test->name);
    for (i = 0; i < source_count; ++i)
        snprintf(objects[i], sizeof objects[i], "%s\\%s-%d.obj", work, test->name, i);
    for (stage = 0; stage <= source_count; ++stage) {
        NtProcessResult result;
        n = 0; command[n++] = compiler;
        if (runtime && *runtime) command[n++] = runtime_arg;
        for (i = 0; i < extra_count; ++i) command[n++] = extra[i];
        if (stage < source_count) {
            command[n++] = "-c"; command[n++] = sources[stage];
        } else {
            for (i = 0; i < source_count; ++i) command[n++] = objects[i];
        }
        command[n++] = "-o";
        command[n++] = stage < source_count ? objects[stage] : executable;
        command[n] = NULL;
        for (i = 0; i < n; ++i) response_argument(&evidence, command[i]);
        DeleteFileA(stage < source_count ? objects[stage] : executable);
        result = nt_run(command, root, timeout_ms, 0);
        nt_buffer_text(&evidence, result.output ? result.output : "");
        if (!result.started || result.timed_out ||
            (stage < source_count ? result.exit_code != 0 || !nt_exists(objects[stage])
                                  : result.exit_code != 1)) {
            printf("FAIL %s: expected link rejection; %s stage %s (exit %lu)\n",
                   test->name, stage < source_count ? "compile" : "link",
                   !result.started ? "did not start" : result.timed_out ? "timed out" : "mismatched",
                   result.exit_code);
            ok = 0;
        }
        nt_process_free(&result);
        if (!ok) break;
    }
    nt_write_file(log, evidence.data, evidence.size);
    if (ok) printf("PASS %s\n", test->name);
    nt_buffer_free(&source_list); nt_buffer_free(&evidence); free(objects);
    return ok;
}

static int run_one(const TestCase *test, const char *compiler, const char *runtime,
                   const char *work, unsigned timeout_ms) {
    TestMeta meta;
    char stem[512], output[NT_PATH], response_path[NT_PATH], at_response[NT_PATH + 2];
    char args_copy[4096], *extra[128];
    const char *compile_command[6];
    NtBuffer response = {0};
    NtProcessResult compile, run;
    int i, extra_count, source_count = 1, ok = 1;
    parse_meta(test->path, &meta);
    if (meta.link_fail) return run_link_failure(test, &meta, compiler, runtime, work, timeout_ms);
    if (*meta.manifest_source) {
        printf("FAIL %s: EXPECT_MANIFEST_SOURCE requires native manifest context\n", test->name);
        return 0;
    }
    strcpy(stem, test->name);
    *strrchr(stem, '.') = 0;
    snprintf(output, sizeof output, "%s\\%s.%s", work, stem, (meta.compile_only || meta.compile_fail) ? "obj" : "exe");
    snprintf(response_path, sizeof response_path, "%s.rsp", output);
    if (runtime && *runtime) {
        char runtime_arg[NT_PATH + 2];
        snprintf(runtime_arg, sizeof runtime_arg, "-B%s", runtime);
        response_argument(&response, runtime_arg);
    }
    strcpy(args_copy, meta.compile_args);
    extra_count = split_arguments(args_copy, extra, 128);
    for (i = 0; i < extra_count; ++i) response_argument(&response, extra[i]);
    response_argument(&response, test->path);
    source_count += append_additional_sources(&response, test, meta.sources);
    if (meta.compile_only || meta.compile_fail) {
        if (source_count != 1) nt_die("compile-only test has multiple sources", test->path);
        response_argument(&response, "-c");
    }
    nt_write_file(response_path, response.data, response.size);
    snprintf(at_response, sizeof at_response, "@%s", response_path);
    compile_command[0] = compiler;
    compile_command[1] = at_response;
    compile_command[2] = "-o";
    compile_command[3] = output;
    compile_command[4] = NULL;
    DeleteFileA(output);
    compile = nt_run(compile_command, root, timeout_ms, 0);
    DeleteFileA(response_path);
    if (!compile.started) {
        printf("FAIL %s: compiler could not start\n", test->name); ok = 0;
    } else if (compile.timed_out) {
        printf("FAIL %s: compiler exceeded %.3f second budget\n", test->name, timeout_ms / 1000.0); ok = 0;
    } else if (meta.compile_fail) {
        if (compile.exit_code != 1) {
            printf("FAIL %s: expected compiler rejection (exit 1), got %lu\n",
                   test->name, compile.exit_code); ok = 0;
        }
    } else if (compile.exit_code != 0) {
        normalize_output(compile.output);
        printf("FAIL %s: compile failed (exit %lu): %s\n", test->name,
               (unsigned long)compile.exit_code, compile.output); ok = 0;
    } else if (!nt_exists(output)) {
        printf("FAIL %s: compilation produced no output\n", test->name); ok = 0;
    } else if (!meta.compile_only) {
        const char *run_command[2] = {output, NULL};
        run = nt_run(run_command, root, timeout_ms, 0);
        normalize_output(run.output);
        if (!run.started || run.timed_out) {
            printf("FAIL %s: runtime did not complete\n", test->name); ok = 0;
        } else if ((int)run.exit_code != meta.expected_exit) {
            printf("FAIL %s: expected exit %d, got %lu; output: %s\n", test->name,
                   meta.expected_exit, (unsigned long)run.exit_code, run.output); ok = 0;
        } else if (*meta.expected_stdout && strcmp(run.output, meta.expected_stdout)) {
            printf("FAIL %s: stdout mismatch; expected '%s', got '%s'\n", test->name,
                   meta.expected_stdout, run.output); ok = 0;
        }
        nt_process_free(&run);
    }
    if (ok) printf("PASS %s\n", test->name);
    nt_process_free(&compile);
    nt_buffer_free(&response);
    return ok;
}

static int run_suite(const char *suite, const char *compiler, const char *runtime,
                     char **selection, int selection_count, const char *tier,
                     int tier_explicit, unsigned timeout_ms) {
    char suite_path[NT_PATH], pass_path[NT_PATH], fail_path[NT_PATH], work[NT_PATH];
    TestList list = {0};
    RegressionCase *cases;
    NtBuffer batch = {0};
    NtProcessResult compile;
    char batch_path[NT_PATH];
    const char *command[4];
    DWORD pid = GetCurrentProcessId();
    unsigned tick = GetTickCount();
    int i, count = 0, passed = 0, failed = 0;
    double compile_seconds = 0, run_seconds = 0;
    nt_join(suite_path, sizeof suite_path, tests_root, suite);
    nt_join(pass_path, sizeof pass_path, suite_path, "pass");
    nt_join(fail_path, sizeof fail_path, suite_path, "fail");
    discover_directory(&list, pass_path, 0);
    discover_directory(&list, fail_path, 1);
    if (!list.count) nt_die("no test files found under suite", suite_path);
    qsort(list.items, list.count, sizeof *list.items, compare_tests);
    cases = nt_alloc(list.count * sizeof *cases);
    snprintf(work, sizeof work, "%s\\build\\native-tests-%lu-%u", root,
             (unsigned long)pid, tick);
    nt_mkdirs(work);
    for (i = 0; i < list.count; ++i) {
        char relative[NT_PATH];
        int pedantic;
        if (!selected(list.items[i].name, selection, selection_count)) continue;
        snprintf(relative, sizeof relative, "%s/%s/%s", suite,
                 list.items[i].fail_directory ? "fail" : "pass", list.items[i].name);
        for (char *p = relative; *p; ++p) if (*p == '\\') *p = '/';
        pedantic = tier_contains(relative);
        if ((!selection_count || tier_explicit) && _stricmp(tier, "all") &&
            (pedantic != !_stricmp(tier, "pedantic"))) continue;
        {
            RegressionCase *item = &cases[count];
            char args_copy[4096], *extra[128];
            int k, extra_count, source_count = 1;
            strcpy(item->path, list.items[i].path); strcpy(item->name, list.items[i].name);
            parse_meta(item->path, &item->meta);
            if (item->meta.link_fail) {
                if (run_link_failure(&list.items[i], &item->meta, compiler, runtime, work, timeout_ms)) passed++;
                else failed++;
                continue;
            }
            if (*item->meta.manifest_source) nt_die("native manifest test context is not implemented", item->path);
            snprintf(item->output, sizeof item->output, "%s\\test-%d.%s", work, count,
                     (item->meta.compile_only || item->meta.compile_fail) ? "obj" : "exe");
            if (runtime && *runtime) {
                char flag[NT_PATH + 2]; snprintf(flag, sizeof flag, "-B%s", runtime); job_argument(&batch, flag);
            }
            strcpy(args_copy, item->meta.compile_args);
            extra_count = split_arguments(args_copy, extra, 128);
            for (k = 0; k < extra_count; ++k) job_argument(&batch, extra[k]);
            job_argument(&batch, item->path);
            source_count += append_additional_job_sources(&batch, item->path, item->meta.sources);
            if (item->meta.compile_only || item->meta.compile_fail) {
                if (source_count != 1) nt_die("compile-only test has multiple sources", item->path);
                job_argument(&batch, "-c");
            }
            job_argument(&batch, "-o"); job_argument(&batch, item->output);
            nt_buffer_text(&batch, "\n"); count++;
        }
    }
    if (!count) {
        if (passed || failed) {
            printf("\nSummary: %d passed, %d failed\n", passed, failed);
            if (!failed) nt_remove_tree(work);
            else printf("Compiler evidence: %s\n", work);
            free(cases); free(list.items); nt_buffer_free(&batch);
            return failed ? 1 : 0;
        }
        printf("No %s tests under %s\n", tier, suite);
        nt_remove_tree(work); free(cases); free(list.items); return selection_count ? 1 : 0;
    }
    nt_join(batch_path, sizeof batch_path, work, "jobs.txt"); nt_write_file(batch_path, batch.data, batch.size);
    command[0] = compiler; command[1] = "--batch-continue"; command[2] = batch_path; command[3] = NULL;
    compile = nt_run(command, root, timeout_ms * count, 0);
    compile_seconds = compile.wall_seconds;
    printf("Using compiler: %s\nRunning suite: %s\n\n", compiler, suite);
    for (i = 0; i < count; ++i) {
        DWORD code;
        char marker[80];
        const char *timing;
        unsigned elapsed;
        unsigned long job_code;
        snprintf(marker, sizeof marker, "# cprime batch end %d ", i + 1);
        timing = strstr(compile.output, marker);
        if (timing && sscanf(timing + strlen(marker), "%lu %u", &job_code, &elapsed) == 2
            && elapsed >= 100)
            printf("SLOW %s: compile %.3fs\n", cases[i].name, elapsed / 1000.0);
        int ok = find_batch_exit(compile.output, i + 1, &code);
        if (!ok || (cases[i].meta.compile_fail ? code != 1 : code != 0) ||
            (!cases[i].meta.compile_fail && !nt_exists(cases[i].output))) {
            printf("FAIL %s: compiler batch result %s\n", cases[i].name, ok ? "mismatched" : "missing");
            failed++; continue;
        }
        if (!cases[i].meta.compile_fail && !cases[i].meta.compile_only) {
            const char *run_command[2] = {cases[i].output, NULL};
            NtProcessResult run = nt_run(run_command, root, timeout_ms, 0);
            run_seconds += run.wall_seconds;
            normalize_output(run.output);
            if (!run.started || run.timed_out || (int)run.exit_code != cases[i].meta.expected_exit ||
                (*cases[i].meta.expected_stdout && strcmp(run.output, cases[i].meta.expected_stdout))) {
                printf("FAIL %s: runtime mismatch (exit %lu): %s\n", cases[i].name,
                       (unsigned long)run.exit_code, run.output); failed++;
                nt_process_free(&run); continue;
            }
            nt_process_free(&run);
        }
        printf("PASS %s\n", cases[i].name); passed++;
    }
    if (failed) {
        char log[NT_PATH];
        nt_join(log, sizeof log, work, "compiler.log");
        nt_write_file(log, compile.output, strlen(compile.output));
        printf("Failure evidence: %s (compiler exit %lu)\n", work,
               (unsigned long)compile.exit_code);
    } else nt_remove_tree(work);
    nt_process_free(&compile); nt_buffer_free(&batch); free(cases); free(list.items);
    printf("\nSummary: %d passed, %d failed\n", passed, failed);
    printf("Time: compile %.3fs, run %.3fs\n", compile_seconds, run_seconds);
    if (selection_count && passed + failed != selection_count)
        nt_die("one or more selected tests were not found", suite);
    return failed ? 1 : 0;
}

typedef struct RegressionGroup {
    const char *suite;
    const char *names[16];
} RegressionGroup;

static void job_argument(NtBuffer *batch, const char *argument) {
    const char *p;
    if (batch->size && batch->data[batch->size - 1] != '\n') nt_buffer_text(batch, " ");
    nt_buffer_text(batch, "\"");
    for (p = argument; *p; ++p) {
        if (*p == '\\' || *p == '"') nt_buffer_text(batch, "\\");
        nt_buffer_append(batch, p, 1);
    }
    nt_buffer_text(batch, "\"");
}

static int append_additional_job_sources(NtBuffer *batch, const char *test_path,
                                         const char *json) {
    char directory[NT_PATH], value[NT_PATH], full[NT_PATH];
    const char *p = json;
    int count = 0;
    if (!*json) return 0;
    if (*p++ != '[') nt_die("EXPECT_SOURCES must be a JSON array", test_path);
    strcpy(directory, test_path); nt_parent(directory);
    while (*p) {
        size_t used = 0;
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (*p == ']') break;
        if (*p++ != '"') nt_die("invalid EXPECT_SOURCES", test_path);
        while (*p && *p != '"') {
            if (*p == '\\' && p[1]) p++;
            if (used + 1 >= sizeof value) nt_die("additional source path too long", test_path);
            value[used++] = *p++;
        }
        if (*p++ != '"') nt_die("invalid EXPECT_SOURCES", test_path);
        value[used] = 0;
        if (strstr(value, "..") || strchr(value, ':') || value[0] == '/' || value[0] == '\\')
            nt_die("unsafe EXPECT_SOURCES path", value);
        nt_join(full, sizeof full, directory, value);
        if (!nt_exists(full)) nt_die("additional source missing", full);
        job_argument(batch, full); count++;
    }
    return count;
}

static int find_batch_exit(const char *output, int job, DWORD *code) {
    char marker[64];
    const char *found;
    unsigned long parsed;
    snprintf(marker, sizeof marker, "# cprime batch end %d ", job);
    found = strstr(output, marker);
    if (!found || sscanf(found + strlen(marker), "%lu", &parsed) != 1) return 0;
    *code = (DWORD)parsed;
    return 1;
}

static int run_regressions(const char *compiler, const char *runtime, unsigned timeout_ms) {
    static const RegressionGroup groups[] = {
        {"c_compat", {"test_abstract_function_pointer_cast.c", "test_winapi_function_pointer_cast.c", "test_nested_pointer_cast_argument.c", "test_fast_unsigned_range_masked_unreachable.c", NULL}},
        {"features/Expressions", {"test_abstract_function_pointer_cast.cpp", "test_parenthesized_functional_construction.cpp", "test_typeinfo_copy.cpp", NULL}},
        {"features/Constructors", {"test_implicit_derived_copy_with_base_constructors.cpp", "test_initializer_list_backing_lifetime.cpp", "test_array_before_explicit_member_initializers.cpp", "test_nrvo_member_template_implicit_move.cpp", NULL}},
        {"features/Statements", {"test_static_string_array_in_branch.cpp", "test_static_string_array_too_long.cpp", "test_namespace_const_unsigned_alias.cpp", NULL}},
        {"features/Includes", {"test_chrono_header_standalone.cpp", NULL}},
        {"features/StdConcurrency", {"test_async_template_member_pointer_result.cpp", NULL}},
        {"features/Templates", {"test_out_of_class_member_read_overloads.cpp", NULL}},
        /* Each pair must stay ordered in one compiler batch: declarations in
           the first input must not leave token identities in the second. */
        {"features/Templates", {"test_deduction_guide_functional_construction.cpp", "test_function_address_linkage_across_inputs.cpp", "test_explicit_static_member_initialization.cpp", "test_static_member_parenthesized_initializer.cpp", NULL}},
        {"features/Templates", {"test_friend_template_specialization_wrong_signature.cpp", "test_local_auto_member_template_operator_constructor_arg.cpp", NULL}},
        {"features/Templates", {"test_member_template_deduction_scaling.cpp", "test_bound_member_function_decltype_sfinae.cpp", "test_conversion_template_owner_lookup.cpp", "test_conversion_probe_constructor_selection.cpp", "test_member_call_argument_storage.cpp", "test_member_deduction_signature_blocks.cpp", "test_member_reference_overload_converted_key.cpp", "test_nested_layout_parameter_scope.cpp", "test_static_member_template_unqualified_specializations.cpp", "test_inherited_variadic_member_linkage.cpp", "test_indirect_default_value_template_nested_alias.cpp", "test_private_dependent_alias_out_of_class_member.cpp", "test_detection_idiom_two_argument_member_enable_if.cpp", "test_member_template_detection_overload_dependent_value.cpp", "test_template_member_scoped_enum_operator_lookup.cpp", NULL}},
        {"features/OperatorOverloads", {"test_braced_argument_reference_overload.cpp", "test_derived_memberwise_move_assignment.cpp", "test_template_braced_reference_overload.cpp", "test_braced_reference_unrelated_types_ambiguous.cpp", "test_braced_reference_conflicting_preferences.cpp", "test_braced_argument_constructor_viability.cpp", "test_braced_argument_requires_viable_constructor.cpp", "test_enum_integral_promotion_ranking.cpp", NULL}},
        {"features/Classes", {"test_reject_identical_strong_definitions.cpp", NULL}},
        /* A rejected deferred initializer must not suppress the next job's body. */
        {"features/Namespaces", {"test_using_overload_address_no_match.cpp", "test_using_overload_address_return_mismatch.cpp", NULL}},
        /* A hard instantiation error must not poison later substitution recovery. */
        {"features/Templates", {"test_substitution_class_body_is_hard_error.cpp", "test_substitution_declaration_scope_and_recovery.cpp", "test_variable_template_pack_expansion.cpp", NULL}},
        {"features/Constructors", {"test_deleted_constructor_elision_and_selection.cpp", NULL}},
        {"features/Functions", {"test_deleted_function_overload_selection.cpp", "test_deleted_function_selected.cpp", NULL}},
        {"features/Templates", {"test_deleted_member_template_selected.cpp", NULL}},
        {"features/Destructors", {"test_deleted_destructor_unused.cpp", "test_deleted_destructor_object.cpp", NULL}}
    };
    RegressionCase *cases;
    NtBuffer batch = {0};
    NtProcessResult compile;
    char work[NT_PATH], batch_path[NT_PATH];
    const char *command[4];
    int i, j, capacity = 0, count = 0, passed = 0, failed = 0;
    for (i = 0; i < (int)(sizeof groups / sizeof groups[0]); ++i)
        for (j = 0; groups[i].names[j]; ++j) capacity++;
    cases = nt_alloc(capacity * sizeof *cases);
    snprintf(work, sizeof work, "%s\\build\\native-regression-%lu-%u", root,
             (unsigned long)GetCurrentProcessId(), GetTickCount());
    nt_mkdirs(work);
    for (i = 0; i < (int)(sizeof groups / sizeof groups[0]); ++i) {
        for (j = 0; groups[i].names[j]; ++j) {
            char args_copy[4096], *extra[128], pass[NT_PATH], fail[NT_PATH];
            int k, extra_count, source_count = 1;
            RegressionCase *item = &cases[count];
            nt_join(pass, sizeof pass, tests_root, groups[i].suite);
            nt_join(pass, sizeof pass, pass, "pass");
            nt_join(item->path, sizeof item->path, pass, groups[i].names[j]);
            if (!nt_exists(item->path)) {
                nt_join(fail, sizeof fail, tests_root, groups[i].suite);
                nt_join(fail, sizeof fail, fail, "fail");
                nt_join(item->path, sizeof item->path, fail, groups[i].names[j]);
            }
            if (!nt_exists(item->path)) nt_die("regression test not found", groups[i].names[j]);
            strcpy(item->name, groups[i].names[j]);
            parse_meta(item->path, &item->meta);
            if (item->meta.link_fail) {
                TestCase test;
                strcpy(test.path, item->path); strcpy(test.name, item->name);
                if (run_link_failure(&test, &item->meta, compiler, runtime, work, timeout_ms)) passed++;
                else failed++;
                continue;
            }
            snprintf(item->output, sizeof item->output, "%s\\regression-%d.%s", work, count,
                     (item->meta.compile_only || item->meta.compile_fail) ? "obj" : "exe");
            if (runtime && *runtime) {
                char flag[NT_PATH + 2]; snprintf(flag, sizeof flag, "-B%s", runtime); job_argument(&batch, flag);
            }
            strcpy(args_copy, item->meta.compile_args);
            extra_count = split_arguments(args_copy, extra, 128);
            for (k = 0; k < extra_count; ++k) job_argument(&batch, extra[k]);
            job_argument(&batch, item->path);
            source_count += append_additional_job_sources(&batch, item->path, item->meta.sources);
            if (item->meta.compile_only || item->meta.compile_fail) {
                if (source_count != 1) nt_die("compile-only test has multiple sources", item->path);
                job_argument(&batch, "-c");
            }
            job_argument(&batch, "-o"); job_argument(&batch, item->output);
            nt_buffer_text(&batch, "\n");
            count++;
        }
    }
    nt_join(batch_path, sizeof batch_path, work, "jobs.txt");
    nt_write_file(batch_path, batch.data, batch.size);
    command[0] = compiler; command[1] = "--batch-continue"; command[2] = batch_path; command[3] = NULL;
    compile = nt_run(command, root, timeout_ms * count, 0);
    printf("Using compiler: %s\nRunning native regression batch: %d tests\n\n", compiler, count);
    for (i = 0; i < count; ++i) {
        DWORD code;
        int ok = find_batch_exit(compile.output, i + 1, &code);
        if (!ok) {
            printf("FAIL %s: missing compiler batch result\n", cases[i].name); failed++; continue;
        }
        if (cases[i].meta.compile_fail ? code != 1 : code != 0) {
            printf("FAIL %s: unexpected compiler exit %lu\n", cases[i].name, (unsigned long)code);
            failed++; continue;
        }
        if (!cases[i].meta.compile_fail && !nt_exists(cases[i].output)) {
            printf("FAIL %s: compilation produced no output\n", cases[i].name); failed++; continue;
        }
        if (!cases[i].meta.compile_fail && !cases[i].meta.compile_only) {
            const char *run_command[2] = {cases[i].output, NULL};
            NtProcessResult run = nt_run(run_command, root, timeout_ms, 0);
            normalize_output(run.output);
            if (!run.started || run.timed_out || (int)run.exit_code != cases[i].meta.expected_exit ||
                (*cases[i].meta.expected_stdout && strcmp(run.output, cases[i].meta.expected_stdout))) {
                printf("FAIL %s: runtime result mismatch (exit %lu): %s\n", cases[i].name,
                       (unsigned long)run.exit_code, run.output); failed++;
                nt_process_free(&run); continue;
            }
            nt_process_free(&run);
        }
        printf("PASS %s\n", cases[i].name); passed++;
    }
    if (!compile.started || compile.timed_out)
        printf("FAIL compiler batch: %s\n", compile.timed_out ? "timeout" : "could not start");
    printf("\nSummary: %d passed, %d failed\n", passed, failed);
    if (failed) {
        char log[NT_PATH];
        nt_join(log, sizeof log, work, "compiler.log");
        nt_write_file(log, compile.output, strlen(compile.output));
        printf("Compiler evidence: %s\n", work);
    } else nt_remove_tree(work);
    nt_process_free(&compile); nt_buffer_free(&batch); free(cases);
    return failed ? 1 : 0;
}

static int run_all_suites(const char *compiler, const char *runtime, const char *tier,
                          unsigned timeout_ms, int list_only);

static int run_runner_checks(const char *compiler, const char *runtime, unsigned timeout_ms) {
    static const struct { const char *name; const char *diagnostic; } checks[] = {
        {"test_valid_without_main.cpp", "compiler batch result mismatched"},
        {"test_invalid_before_link.cpp", "compile stage mismatched (exit 1)"},
        {"test_valid_link.cpp", "link stage mismatched (exit 0)"}
    };
    char runner[NT_PATH];
    int i, failed = 0;
    nt_join(runner, sizeof runner, tests_root, "test.exe");
    for (i = 0; i < (int)(sizeof checks / sizeof checks[0]); ++i) {
        const char *command[12];
        int n = 0;
        command[n++] = runner; command[n++] = "-CompilerPath"; command[n++] = compiler;
        if (runtime && *runtime) {
            command[n++] = "-RuntimeRoot"; command[n++] = runtime;
        }
        command[n++] = "-Suite"; command[n++] = "tools/fixtures/negative_runner";
        command[n++] = "-Select"; command[n++] = checks[i].name; command[n] = NULL;
        NtProcessResult result = nt_run(command, root, timeout_ms * 4, 0);
        int ok = result.started && !result.timed_out && result.exit_code == 1 &&
                 strstr(result.output, checks[i].diagnostic) &&
                 strstr(result.output, "Summary: 0 passed, 1 failed");
        printf("%s runner rejects false expectation: %s\n", ok ? "PASS" : "FAIL", checks[i].name);
        if (!ok) { puts(result.output); failed++; }
        nt_process_free(&result);
    }
    return failed != 0;
}

static void usage(void) {
    puts("test.exe -Suite NAME [-Select TEST ...] [-CompilerPath PATH] [-RuntimeRoot PATH] [-Tier fast|pedantic|all] [-Timeout SECONDS]\n"
         "test.exe -All [-CompilerPath PATH] [-RuntimeRoot PATH] [-Tier fast|pedantic|all]\n"
         "test.exe -Regression [-CompilerPath PATH] [-RuntimeRoot PATH]\n"
         "test.exe -Checks [-CompilerPath PATH] [-RuntimeRoot PATH]\n"
         "test.exe -RunnerChecks [-CompilerPath PATH] [-RuntimeRoot PATH]\n"
         "-Checks is the fast CPC-only publication/development gate; external ABI checks live in test-msvc.exe.");
}

static int run_checks(const char *compiler, const char *runtime, unsigned timeout_ms) {
    char maintenance[NT_PATH], helper_source[NT_PATH], helper_exe[NT_PATH];
    const char *policy[3];
    NtProcessResult result;
    int failed = 0;
    if (run_runner_checks(compiler, runtime, timeout_ms)) failed = 1;
    nt_join(maintenance, sizeof maintenance, root, "scripts\\maintenance.exe");
    policy[0] = maintenance; policy[1] = "-CheckSources"; policy[2] = NULL;
    result = nt_run(policy, root, 30000, 1);
    if (!result.started || result.timed_out || result.exit_code) failed = 1;
    nt_process_free(&result);
    nt_join(helper_source, sizeof helper_source, root, "Tests\\tools\\test_compiler_cold_paths.c");
    nt_join(helper_exe, sizeof helper_exe, root, "build\\test-compiler-cold-paths.exe");
    {
        char i1[NT_PATH + 3], i2[NT_PATH + 3], i3[NT_PATH + 3], i4[NT_PATH + 3],
             i5[NT_PATH + 3], i6[NT_PATH + 3], i7[NT_PATH + 3];
        const char *compile[24], *run[2]; int n = 0;
#define ROOT_INCLUDE(slot, relative) do { char p[NT_PATH]; nt_join(p,sizeof p,root,relative); snprintf(slot,sizeof slot,"-I%s",p); compile[n++]=slot; } while (0)
        compile[n++] = compiler; compile[n++] = "-O2";
        ROOT_INCLUDE(i1,"include\\runtime"); ROOT_INCLUDE(i2,"include\\cprime");
        ROOT_INCLUDE(i3,"third-party\\win32-sdk\\include"); ROOT_INCLUDE(i4,"third-party\\win32-sdk\\include\\winapi");
        ROOT_INCLUDE(i5,"src\\compiler\\frontend"); ROOT_INCLUDE(i6,"src\\compiler\\middleend");
        ROOT_INCLUDE(i7,"src\\compiler\\backend\\x64");
        compile[n++] = "-DCPRIME_TARGET_PE"; compile[n++] = "-DCPRIME_TARGET_X86_64";
        compile[n++] = helper_source; compile[n++] = "-o"; compile[n++] = helper_exe; compile[n] = NULL;
        result = nt_run(compile, root, 120000, 0);
        if (!result.started || result.timed_out || result.exit_code) {
            fprintf(stderr,"FAIL compiler cold-path helper build: %s\n",result.output); failed = 1;
        }
        nt_process_free(&result);
        if (!failed) {
            run[0] = helper_exe; run[1] = NULL; result = nt_run(run, root, 30000, 1);
            if (!result.started || result.timed_out || result.exit_code) failed = 1;
            else puts("PASS compiler cold paths");
            nt_process_free(&result);
        }
    }
    if (run_regressions(compiler, runtime, timeout_ms)) failed = 1;
    nt_join(helper_source, sizeof helper_source, root, "Tests\\tools\\test_process_path.c");
    nt_join(helper_exe, sizeof helper_exe, root, "build\\test-process-path.exe");
    {
        const char *compile[] = {compiler, "-O2", helper_source, "-o", helper_exe, NULL};
        const char *run[] = {helper_exe, NULL};
        int helper_ok = 1;
        result = nt_run(compile, root, 120000, 0);
        if (!result.started || result.timed_out || result.exit_code) {
            fprintf(stderr, "FAIL native process PATH helper build: %s\n", result.output);
            helper_ok = 0; failed = 1;
        }
        nt_process_free(&result);
        if (helper_ok) {
            result = nt_run(run, root, 30000, 1);
            if (!result.started || result.timed_out || result.exit_code) failed = 1;
            nt_process_free(&result);
        }
    }
    if (run_all_suites(compiler, runtime, "fast", timeout_ms, 0)) failed = 1;
    return failed;
}

static int add_suite(char suites[][512], int count, const char *name) {
    if (count >= 128 || strlen(name) >= 512) nt_die("too many test suites", name);
    strcpy(suites[count], name); return count + 1;
}

static int compare_suite_names(const void *left, const void *right) {
    return _stricmp((const char *)left, (const char *)right);
}

static int run_all_suites(const char *compiler, const char *runtime, const char *tier,
                          unsigned timeout_ms, int list_only) {
    char suites[128][512], features[NT_PATH], pattern[NT_PATH];
    WIN32_FIND_DATAA entry;
    HANDLE find;
    int count = 0, i, failed = 0;
    count = add_suite(suites, count, "c_compat");
    count = add_suite(suites, count, "debug");
    count = add_suite(suites, count, "payload");
    nt_join(features, sizeof features, tests_root, "features");
    nt_join(pattern, sizeof pattern, features, "*");
    find = FindFirstFileA(pattern, &entry);
    if (find == INVALID_HANDLE_VALUE) nt_die("cannot enumerate feature suites", features);
    do {
        if ((entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(entry.cFileName, ".") && strcmp(entry.cFileName, "..")) {
            char name[512]; snprintf(name, sizeof name, "features/%s", entry.cFileName);
            count = add_suite(suites, count, name);
        }
    } while (FindNextFileA(find, &entry));
    FindClose(find);
    qsort(suites, count, sizeof suites[0], compare_suite_names);
    if (list_only) { for (i = 0; i < count; ++i) puts(suites[i]); return 0; }
    for (i = 0; i < count; ++i)
        if (run_suite(suites[i], compiler, runtime, NULL, 0, tier, 1, timeout_ms)) failed++;
    printf("Suite summary: %d passed, %d failed\n", count - failed, failed);
    return failed ? 1 : 0;
}

int main(int argc, char **argv) {
    char module[NT_PATH], compiler[NT_PATH], runtime[NT_PATH] = "";
    const char *suite = "c_compat", *tier = "fast";
    char *selection[256];
    int selection_count = 0, regression = 0, checks = 0, runner_checks = 0, all = 0, list_only = 0, tier_explicit = 0, i;
    unsigned timeout_ms = 5000;
    nt_module_directory(module, sizeof module);
    strcpy(tests_root, module);
    strcpy(root, module);
    nt_parent(root);
    nt_join(compiler, sizeof compiler, root, "cpc.exe");
    for (i = 1; i < argc; ++i) {
        if (!_stricmp(argv[i], "-Suite") && i + 1 < argc) suite = argv[++i];
        else if (!_stricmp(argv[i], "-CompilerPath") && i + 1 < argc) {
            if (!GetFullPathNameA(argv[++i], sizeof compiler, compiler, NULL)) nt_die("invalid compiler path", argv[i]);
        } else if (!_stricmp(argv[i], "-RuntimeRoot") && i + 1 < argc) {
            if (!GetFullPathNameA(argv[++i], sizeof runtime, runtime, NULL)) nt_die("invalid runtime path", argv[i]);
        } else if (!_stricmp(argv[i], "-Tier") && i + 1 < argc) { tier = argv[++i]; tier_explicit = 1; }
        else if (!_stricmp(argv[i], "-Timeout") && i + 1 < argc) timeout_ms = (unsigned)(atof(argv[++i]) * 1000.0);
        else if (!_stricmp(argv[i], "-Regression")) regression = 1;
        else if (!_stricmp(argv[i], "-Checks")) checks = 1;
        else if (!_stricmp(argv[i], "-RunnerChecks")) runner_checks = 1;
        else if (!_stricmp(argv[i], "-All")) all = 1;
        else if (!_stricmp(argv[i], "-List")) { all = 1; list_only = 1; }
        else if (!_stricmp(argv[i], "-Select")) {
            while (i + 1 < argc && argv[i + 1][0] != '-') selection[selection_count++] = argv[++i];
        } else if (!_stricmp(argv[i], "-Help") || !_stricmp(argv[i], "--help")) { usage(); return 0; }
        else { usage(); nt_die("unknown test option", argv[i]); }
    }
    if (!nt_exists(compiler)) nt_die("compiler not found", compiler);
    if (runner_checks) return run_runner_checks(compiler, runtime, timeout_ms);
    if (checks) return run_checks(compiler, runtime, timeout_ms);
    if (regression) return run_regressions(compiler, runtime, timeout_ms);
    if (all) return run_all_suites(compiler, runtime, tier, timeout_ms, list_only);
    return run_suite(suite, compiler, runtime, selection, selection_count, tier, tier_explicit, timeout_ms);
}
