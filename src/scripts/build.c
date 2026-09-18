#include "common/native_tool.h"

static const char *runtime_sources[] = {
    "src\\runtime\\generic\\libcprime1.c",
    "src\\runtime\\windows\\crt1.c",
    "src\\runtime\\windows\\crt1w.c",
    "src\\runtime\\windows\\wincrt1.c",
    "src\\runtime\\windows\\wincrt1w.c",
    "src\\runtime\\windows\\dllcrt1.c",
    "src\\runtime\\windows\\dllmain.c",
    "src\\runtime\\generic\\stdatomic.c",
    "src\\runtime\\generic\\builtin.c",
    "src\\runtime\\windows\\wincompat.c",
    "src\\runtime\\windows\\regex.c",
    "src\\third-party\\quickjs-regexp\\cutils.c",
    "src\\third-party\\quickjs-regexp\\libunicode.c",
    "src\\third-party\\quickjs-regexp\\libregexp.c",
    "src\\runtime\\windows\\ucrt_stdio.c",
    "src\\runtime\\windows\\ucrt_exit.c",
    "src\\runtime\\windows\\ucrt_onexit.c",
    "src\\runtime\\windows\\exception.c",
    "src\\runtime\\windows\\rtti.cpp",
    "src\\runtime\\windows\\system_error.cpp",
    "src\\runtime\\windows\\new_delete.cpp",
    "src\\runtime\\windows\\winintrin.S",
    "src\\runtime\\windows\\atomic.S",
    "src\\runtime\\windows\\setjmp.S",
    "src\\runtime\\windows\\chkstk.S"
};

static const char *runtime_names[] = {
    "libcprime1", "crt1", "crt1w", "wincrt1", "wincrt1w", "dllcrt1",
    "dllmain", "stdatomic", "builtin", "wincompat", "regex", "cutils",
    "libunicode", "libregexp", "ucrt_stdio", "ucrt_exit", "ucrt_onexit",
    "exception", "rtti", "system_error", "new_delete", "winintrin", "atomic",
    "setjmp", "chkstk"
};

static const char *extra_sources[] = {
    "src\\runtime\\generic\\bcheck.c",
    "src\\runtime\\generic\\bt-exe.c",
    "src\\runtime\\generic\\bt-log.c",
    "src\\runtime\\generic\\bt-dll.c",
    "src\\runtime\\generic\\runmain.c"
};

static const char *extra_names[] = {"bcheck", "bt-exe", "bt-log", "bt-dll", "runmain"};

typedef struct BuildPaths {
    char root[NT_PATH];
    char toolchain[NT_PATH];
    char cpc[NT_PATH];
    char build[NT_PATH];
    char compiler[NT_PATH];
    char bootstrap[NT_PATH];
    char candidate[NT_PATH];
} BuildPaths;

static int build_map;

static void batch_arg(NtBuffer *line, const char *argument) {
    const char *p;
    if (line->size && line->data[line->size - 1] != '\n') nt_buffer_text(line, " ");
    nt_buffer_text(line, "\"");
    for (p = argument; *p; ++p) {
        if (*p == '\\' || *p == '"') nt_buffer_text(line, "\\");
        nt_buffer_append(line, p, 1);
    }
    nt_buffer_text(line, "\"");
}

static void batch_end(NtBuffer *batch) { nt_buffer_text(batch, "\n"); }

static void absolute_path(char *out, size_t capacity, const BuildPaths *paths, const char *relative) {
    nt_join(out, capacity, paths->root, relative);
}

/* Every batch runs with the tree root as its working directory, so the compiler
   can be handed tree-relative sources and include directories. An absolute path
   here is written into the compiler and the packaged runtime through __FILE__,
   which would make the published binaries depend on where this clone lives and
   would hand every machine a different compiler for identical sources. */
static void rooted_path(char *out, size_t capacity, const char *relative) {
    if (strlen(relative) + 1 > capacity) nt_die("path too long", relative);
    strcpy(out, relative);
}

static void add_common_flags(NtBuffer *batch, const BuildPaths *paths, const char *runtime_root) {
    char value[NT_PATH + 4];
    (void)paths;
    snprintf(value, sizeof value, "-B%s", runtime_root); batch_arg(batch, value);
    rooted_path(value, sizeof value, "src/include/runtime"); {
        char flag[NT_PATH + 4]; snprintf(flag, sizeof flag, "-I%s", value); batch_arg(batch, flag);
    }
    rooted_path(value, sizeof value, "src/include/cprime"); {
        char flag[NT_PATH + 4]; snprintf(flag, sizeof flag, "-I%s", value); batch_arg(batch, flag);
    }
    rooted_path(value, sizeof value, "src/third-party/win32-sdk/include"); {
        char flag[NT_PATH + 4]; snprintf(flag, sizeof flag, "-I%s", value); batch_arg(batch, flag);
    }
    rooted_path(value, sizeof value, "src/third-party/win32-sdk/include/winapi"); {
        char flag[NT_PATH + 4]; snprintf(flag, sizeof flag, "-I%s", value); batch_arg(batch, flag);
    }
    rooted_path(value, sizeof value, "src/compiler/frontend"); {
        char flag[NT_PATH + 4]; snprintf(flag, sizeof flag, "-I%s", value); batch_arg(batch, flag);
    }
    rooted_path(value, sizeof value, "src/compiler/middleend"); {
        char flag[NT_PATH + 4]; snprintf(flag, sizeof flag, "-I%s", value); batch_arg(batch, flag);
    }
    rooted_path(value, sizeof value, "src/compiler/backend/x64"); {
        char flag[NT_PATH + 4]; snprintf(flag, sizeof flag, "-I%s", value); batch_arg(batch, flag);
    }
    batch_arg(batch, "-I.");
    batch_arg(batch, "-DCPRIME_TARGET_PE");
    batch_arg(batch, "-DCPRIME_TARGET_X86_64");
}

static void copy_definitions(const BuildPaths *paths, const char *destination) {
    WIN32_FIND_DATAA entry;
    HANDLE find;
    char source_dir[NT_PATH], pattern[NT_PATH], source[NT_PATH], target[NT_PATH];
    absolute_path(source_dir, sizeof source_dir, paths, "src/third-party/win32-sdk/lib");
    nt_mkdirs(destination);
    nt_join(pattern, sizeof pattern, source_dir, "*.def");
    find = FindFirstFileA(pattern, &entry);
    if (find == INVALID_HANDLE_VALUE) nt_die("no SDK definitions found", source_dir);
    do {
        nt_join(source, sizeof source, source_dir, entry.cFileName);
        nt_join(target, sizeof target, destination, entry.cFileName);
        nt_copy_file(source, target);
    } while (FindNextFileA(find, &entry));
    FindClose(find);
}

static void add_runtime_jobs(NtBuffer *batch, const BuildPaths *paths,
                             const char *host_runtime, const char *object_dir,
                             const char *library_dir, int bootstrap) {
    int i;
    char source[NT_PATH], output[NT_PATH], quickjs[NT_PATH + 4], runtime_inc[NT_PATH + 4];
    for (i = 0; i < (int)(sizeof runtime_sources / sizeof runtime_sources[0]); ++i) {
        /* The seed compiler is a C program.  C++ library support is built only
           after that compiler exists, so a C-only seed host (TCC) never has to
           parse rtti.cpp or new_delete.cpp. */
        if (bootstrap && (!strcmp(runtime_names[i], "rtti") ||
                          !strcmp(runtime_names[i], "system_error") ||
                          !strcmp(runtime_names[i], "new_delete"))) continue;
        add_common_flags(batch, paths, host_runtime);
        batch_arg(batch, "-m64");
        batch_arg(batch, "-c");
        if (bootstrap && !strcmp(runtime_names[i], "chkstk"))
            batch_arg(batch, "-DCPRIME_BOOTSTRAP_CHKSTK");
        if (!strcmp(runtime_names[i], "regex")) {
            rooted_path(source, sizeof source, "src/third-party/quickjs-regexp");
            snprintf(quickjs, sizeof quickjs, "-I%s", source);
            batch_arg(batch, quickjs);
        }
        if (!strcmp(runtime_names[i], "exception") || !strcmp(runtime_names[i], "rtti") ||
            !strcmp(runtime_names[i], "new_delete")) {
            rooted_path(source, sizeof source, "src/include/runtime");
            snprintf(runtime_inc, sizeof runtime_inc, "-I%s", source);
            batch_arg(batch, runtime_inc);
        }
        rooted_path(source, sizeof source, runtime_sources[i]);
        nt_join(output, sizeof output, object_dir, runtime_names[i]);
        strcat(output, ".o");
        batch_arg(batch, source);
        batch_arg(batch, "-o"); batch_arg(batch, output);
        batch_end(batch);
    }
    batch_arg(batch, "-ar"); batch_arg(batch, "rcs");
    nt_join(output, sizeof output, library_dir, "libcprime1.a");
    batch_arg(batch, output);
    for (i = 0; i < (int)(sizeof runtime_names / sizeof runtime_names[0]); ++i) {
        if (bootstrap && (!strcmp(runtime_names[i], "rtti") ||
                          !strcmp(runtime_names[i], "system_error") ||
                          !strcmp(runtime_names[i], "new_delete"))) continue;
        nt_join(output, sizeof output, object_dir, runtime_names[i]);
        strcat(output, ".o"); batch_arg(batch, output);
    }
    batch_end(batch);
}

static void add_candidate_job(NtBuffer *batch, const BuildPaths *paths) {
    char source[NT_PATH], output[NT_PATH], map[NT_PATH + 16];
    /* The fast compiler loop links against the last validated root runtime.
       Runtime regeneration is a package-cache miss, not a prerequisite for
       compiling this all-C compiler translation unit. */
    add_common_flags(batch, paths, paths->toolchain);
    batch_arg(batch, "-O2");
    if (build_map) {
        nt_join(map, sizeof map, paths->compiler, "cpc.map");
        char flag[NT_PATH + 16]; snprintf(flag, sizeof flag, "-Wl,-Map=%s", map); batch_arg(batch, flag);
    }
    rooted_path(source, sizeof source, "src/compiler/driver/cprime.c");
    batch_arg(batch, source);
    nt_join(output, sizeof output, paths->compiler, "cpc.exe");
    batch_arg(batch, "-o"); batch_arg(batch, output);
    batch_end(batch);
}

static void add_extra_jobs(NtBuffer *batch, const BuildPaths *paths,
                           const char *object_dir, const char *library_dir) {
    int i;
    char source[NT_PATH], output[NT_PATH];
    for (i = 0; i < (int)(sizeof extra_sources / sizeof extra_sources[0]); ++i) {
        add_common_flags(batch, paths, paths->compiler);
        batch_arg(batch, "-m64"); batch_arg(batch, "-c");
        if (!strcmp(extra_names[i], "bcheck")) {
            batch_arg(batch, "-bt");
            batch_arg(batch, "-I.");
        }
        rooted_path(source, sizeof source, extra_sources[i]); batch_arg(batch, source);
        nt_join(output, sizeof output, library_dir, extra_names[i]); strcat(output, ".o");
        batch_arg(batch, "-o"); batch_arg(batch, output); batch_end(batch);
    }
}

static NtProcessResult run_batch(const char *compiler, const char *cwd,
                                 const char *path, NtBuffer *batch) {
    char at[NT_PATH + 2];
    const char *command[3];
    nt_write_file(path, batch->data, batch->size);
    snprintf(at, sizeof at, "@%s", path);
    command[0] = compiler; command[1] = at; command[2] = NULL;
    return nt_run(command, cwd, 300000, 1);
}

static void initialize_paths(BuildPaths *paths) {
    nt_tree_root(paths->root, sizeof paths->root);
    nt_join(paths->toolchain, sizeof paths->toolchain, paths->root, "src");
    nt_join(paths->cpc, sizeof paths->cpc, paths->root, "cpc.exe");
    nt_join(paths->build, sizeof paths->build, paths->toolchain, "build");
    nt_join(paths->compiler, sizeof paths->compiler, paths->build, "compiler");
    nt_join(paths->bootstrap, sizeof paths->bootstrap, paths->build, "bootstrap");
    nt_join(paths->candidate, sizeof paths->candidate, paths->compiler, "cpc.exe");
}

static int checked_process(const char *label, NtProcessResult *result) {
    if (!result->started || result->timed_out || result->exit_code) {
        fprintf(stderr, "ERROR: %s failed%s (exit %lu)\n", label,
                result->timed_out ? " by timeout" : "", (unsigned long)result->exit_code);
        nt_process_free(result);
        return 0;
    }
    nt_process_free(result);
    return 1;
}

typedef struct PathList {
    char **items;
    int count;
    int capacity;
} PathList;

static void path_list_add(PathList *list, const char *path) {
    if (list->count == list->capacity) {
        int capacity = list->capacity ? list->capacity * 2 : 256;
        char **next = realloc(list->items, capacity * sizeof *next);
        if (!next) nt_die("out of memory", "");
        list->items = next; list->capacity = capacity;
    }
    list->items[list->count++] = nt_strdup(path);
}

static int compare_path_items(const void *left, const void *right) {
    return _stricmp(*(const char *const *)left, *(const char *const *)right);
}

static void collect_files(PathList *list, const char *base, const char *directory) {
    WIN32_FIND_DATAA entry;
    HANDLE find;
    char pattern[NT_PATH], child[NT_PATH];
    nt_join(pattern, sizeof pattern, directory, "*");
    find = FindFirstFileA(pattern, &entry);
    if (find == INVALID_HANDLE_VALUE) nt_die("cannot enumerate package stage", directory);
    do {
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        nt_join(child, sizeof child, directory, entry.cFileName);
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            nt_die("refusing redirected package entry", child);
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) collect_files(list, base, child);
        else {
            const char *relative = child + strlen(base);
            while (*relative == '\\' || *relative == '/') relative++;
            path_list_add(list, relative);
        }
    } while (FindNextFileA(find, &entry));
    FindClose(find);
}

static int run_simple(const char *label, const char *cwd, unsigned timeout,
                      const char *const *command, int echo) {
    NtProcessResult result = nt_run(command, cwd, timeout, echo);
    return checked_process(label, &result);
}

static void overlay_runtime_headers(const BuildPaths *paths, const char *stage_include) {
    WIN32_FIND_DATAA entry;
    HANDLE find;
    char runtime[NT_PATH], pattern[NT_PATH], duplicate[NT_PATH], text[1024];
    absolute_path(runtime, sizeof runtime, paths, "src\\include\\runtime");
    nt_join(pattern, sizeof pattern, runtime, "*.h");
    find = FindFirstFileA(pattern, &entry);
    if (find == INVALID_HANDLE_VALUE) nt_die("cannot enumerate runtime headers", runtime);
    do {
        char winapi[NT_PATH];
        nt_join(winapi, sizeof winapi, stage_include, "winapi");
        nt_join(duplicate, sizeof duplicate, winapi, entry.cFileName);
        if (nt_exists(duplicate)) {
            snprintf(text, sizeof text, "#include \"../%s\"\n", entry.cFileName);
            nt_write_file(duplicate, text, strlen(text));
        }
    } while (FindNextFileA(find, &entry));
    FindClose(find);
}

static int prepare_package(const BuildPaths *paths) {
    char cache[NT_PATH], stage[NT_PATH], tools[NT_PATH], helper[NT_PATH], helper_source[NT_PATH],
         stage_include[NT_PATH], stage_lib[NT_PATH], source[NT_PATH], target[NT_PATH],
         abi[NT_PATH], abi_c[NT_PATH], abi_s[NT_PATH], archive[NT_PATH], manifest[NT_PATH],
         payload[NT_PATH], cached_payload[NT_PATH], cached_helper[NT_PATH], bflag[NT_PATH + 4];
    char runtime_flag[NT_PATH + 4], sdk_flag[NT_PATH + 4], winapi_flag[NT_PATH + 4];
    const char *command[16];
    PathList files = {0};
    NtBuffer list = {0};
    int i;
    nt_join(cache, sizeof cache, paths->build, "portable-cache");
    nt_join(stage, sizeof stage, paths->build, "portable-stage-native");
    if (nt_exists(stage)) nt_remove_tree(stage);
    nt_join(tools, sizeof tools, stage, "tools");
    nt_join(stage_include, sizeof stage_include, stage, "include");
    nt_join(stage_lib, sizeof stage_lib, stage, "lib");
    nt_mkdirs(tools); nt_mkdirs(stage_include); nt_mkdirs(stage_lib); nt_mkdirs(cache);
    nt_join(helper, sizeof helper, tools, "portable-payload.exe");
    absolute_path(helper_source, sizeof helper_source, paths, "src\\tools\\portable_payload.c");
    snprintf(bflag, sizeof bflag, "-B%s", paths->compiler);
    absolute_path(source, sizeof source, paths, "src\\include\\runtime");
    snprintf(runtime_flag, sizeof runtime_flag, "-I%s", source);
    absolute_path(source, sizeof source, paths, "src\\third-party\\win32-sdk\\include");
    snprintf(sdk_flag, sizeof sdk_flag, "-I%s", source);
    absolute_path(source, sizeof source, paths, "src\\third-party\\win32-sdk\\include\\winapi");
    snprintf(winapi_flag, sizeof winapi_flag, "-I%s", source);
    command[0] = paths->candidate; command[1] = bflag; command[2] = runtime_flag;
    command[3] = sdk_flag; command[4] = winapi_flag; command[5] = "-O2";
    command[6] = helper_source; command[7] = "-o"; command[8] = helper; command[9] = NULL;
    if (!run_simple("build portable helper", paths->root, 120000, command, 1)) goto failed;
    absolute_path(source, sizeof source, paths, "src\\third-party\\win32-sdk\\include");
    nt_copy_tree(source, stage_include);
    absolute_path(source, sizeof source, paths, "src\\include\\runtime");
    nt_copy_tree(source, stage_include);
    overlay_runtime_headers(paths, stage_include);
    command[0] = helper; command[1] = "headers"; command[2] = stage_include; command[3] = NULL;
    if (!run_simple("normalize portable headers", paths->root, 120000, command, 1)) goto failed;
    absolute_path(source, sizeof source, paths, "src\\third-party\\win32-sdk\\lib");
    nt_copy_tree(source, stage_lib);
    {
        char runtime_lib[NT_PATH];
        nt_join(runtime_lib, sizeof runtime_lib, paths->compiler, "lib");
        nt_copy_tree(runtime_lib, stage_lib);
    }
    nt_join(abi, sizeof abi, tools, "check-runtime-abi.exe");
    absolute_path(abi_c, sizeof abi_c, paths, "src\\tools\\check_runtime_abi.c");
    absolute_path(abi_s, sizeof abi_s, paths, "src\\tools\\check_runtime_abi.S");
    nt_join(archive, sizeof archive, stage_lib, "libcprime1.a");
    snprintf(bflag, sizeof bflag, "-B%s", stage);
    command[0] = paths->candidate; command[1] = bflag; command[2] = abi_c; command[3] = abi_s;
    command[4] = archive; command[5] = "-o"; command[6] = abi; command[7] = NULL;
    if (!run_simple("build runtime ABI check", paths->root, 120000, command, 1)) goto failed;
    command[0] = abi; command[1] = NULL;
    if (!run_simple("runtime ABI check", paths->root, 10000, command, 1)) goto failed;
    collect_files(&files, stage, stage_include);
    collect_files(&files, stage, stage_lib);
    qsort(files.items, files.count, sizeof *files.items, compare_path_items);
    for (i = 0; i < files.count; ++i) {
        char normalized[NT_PATH], *p;
        strcpy(normalized, files.items[i]);
        for (p = normalized; *p; ++p) if (*p == '\\') *p = '/';
        nt_buffer_text(&list, normalized); nt_buffer_text(&list, "\n");
    }
    nt_join(manifest, sizeof manifest, tools, "files.txt");
    nt_write_file(manifest, list.data, list.size);
    nt_join(payload, sizeof payload, tools, "payload.bin");
    command[0] = helper; command[1] = "prepare"; command[2] = payload;
    command[3] = stage; command[4] = manifest; command[5] = NULL;
    if (!run_simple("prepare portable payload", paths->root, 120000, command, 1)) goto failed;
    nt_join(cached_payload, sizeof cached_payload, cache, "payload.bin");
    nt_join(cached_helper, sizeof cached_helper, cache, "portable-payload.exe");
    nt_copy_file(payload, cached_payload); nt_copy_file(helper, cached_helper);
    command[0] = cached_helper; command[1] = "seal"; command[2] = cache;
    command[3] = paths->root; command[4] = stage_lib; command[5] = NULL;
    /* Seal against the actual build runtime path, not the temporary copy. */
    {
        char runtime_lib[NT_PATH];
        nt_join(runtime_lib, sizeof runtime_lib, paths->compiler, "lib");
        command[4] = runtime_lib;
        if (!run_simple("seal portable cache", paths->root, 120000, command, 1)) goto failed;
    }
    command[0] = cached_helper; command[1] = "attach"; command[2] = paths->candidate;
    command[3] = cached_payload; command[4] = NULL;
    if (!run_simple("attach portable payload", paths->root, 120000, command, 1)) goto failed;
    for (i = 0; i < files.count; ++i) free(files.items[i]);
    free(files.items); nt_buffer_free(&list); nt_remove_tree(stage);
    puts("Portable payload: prepared");
    return 1;
failed:
    for (i = 0; i < files.count; ++i) free(files.items[i]);
    free(files.items); nt_buffer_free(&list);
    if (nt_exists(stage)) nt_remove_tree(stage);
    return 0;
}

static int package_cached(const BuildPaths *paths) {
    char helper[NT_PATH], cache[NT_PATH], library[NT_PATH];
    const char *command[7];
    NtProcessResult result;
    nt_join(cache, sizeof cache, paths->build, "portable-cache");
    nt_join(helper, sizeof helper, cache, "portable-payload.exe");
    nt_join(library, sizeof library, paths->compiler, "lib");
    if (nt_exists(helper)) {
        command[0] = helper; command[1] = "cached"; command[2] = paths->candidate;
        command[3] = cache; command[4] = paths->root; command[5] = library; command[6] = NULL;
        result = nt_run(command, paths->root, 120000, 1);
        if (result.started && !result.timed_out && result.exit_code == 0) {
            nt_process_free(&result);
            return 1;
        }
        nt_process_free(&result);
    }
    return 0;
}

static int validate_candidate(const BuildPaths *paths) {
    char runner[NT_PATH];
    const char *command[8];
    NtProcessResult result;
    nt_join(runner, sizeof runner, paths->root, "Compatibility/tests/test.exe");
    if (!nt_exists(runner)) nt_die("native regression runner missing", runner);
    command[0] = runner; command[1] = "-Regression"; command[2] = "-CompilerPath";
    command[3] = paths->candidate; command[4] = NULL;
    result = nt_run(command, paths->root, 120000, 1);
    return checked_process("candidate regression gate", &result);
}

static int publish_candidate(const BuildPaths *paths) {
    char backup[NT_PATH];
    nt_join(backup, sizeof backup, paths->root, "cpc.exe.bak");
    DeleteFileA(backup);
    if (!MoveFileExA(paths->cpc, backup, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        fprintf(stderr, "ERROR: could not back up root cpc.exe\n"); return 0;
    }
    if (!MoveFileExA(paths->candidate, paths->cpc, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        fprintf(stderr, "ERROR: could not publish candidate (Win32 %lu); restoring root cpc.exe\n",
                (unsigned long)GetLastError());
        MoveFileExA(backup, paths->cpc, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        return 0;
    }
    DeleteFileA(backup);
    return 1;
}

static int build_compiler(const BuildPaths *paths, double *bootstrap_seconds) {
    NtBuffer batch = {0};
    NtProcessResult result;
    char response[NT_PATH];
    double begin;
    add_candidate_job(&batch, paths);
    nt_join(response, sizeof response, paths->compiler, "compiler-job.txt");
    begin = nt_seconds();
    result = run_batch(paths->cpc, paths->root, response, &batch);
    *bootstrap_seconds = nt_seconds() - begin;
    nt_buffer_free(&batch);
    if (!checked_process("root CPC C compiler build", &result)) return 0;
    return 1;
}

static int build_candidate_runtime(const BuildPaths *paths, double *seconds) {
    NtBuffer batch = {0};
    NtProcessResult result;
    char object_dir[NT_PATH], library_dir[NT_PATH], archive[NT_PATH], response[NT_PATH];
    double begin;
    nt_join(object_dir, sizeof object_dir, paths->compiler, "obj");
    nt_join(library_dir, sizeof library_dir, paths->compiler, "lib");
    nt_mkdirs(object_dir); nt_mkdirs(library_dir);
    copy_definitions(paths, library_dir);
    nt_join(archive, sizeof archive, library_dir, "libcprime1.a"); DeleteFileA(archive);
    add_runtime_jobs(&batch, paths, paths->compiler, object_dir, library_dir, 0);
    add_extra_jobs(&batch, paths, object_dir, library_dir);
    nt_join(response, sizeof response, paths->compiler, "runtime-jobs.txt");
    begin = nt_seconds();
    result = run_batch(paths->candidate, paths->root, response, &batch);
    *seconds = nt_seconds() - begin;
    nt_buffer_free(&batch);
    return checked_process("candidate packaged runtime build", &result);
}

static void usage(void) {
    puts("build.exe [-RebuildRuntime] [-Map] [-NoPack] [-NoValidate] [-NoPublish]\n"
         "Default: serial C-only CPC self-host, cached package/runtime, validate and publish.\n"
         "Runtime/SDK/package input changes automatically rebuild the packaged runtime.\n"
         "Diagnostic switches never publish unless packaging and validation remain enabled.");
}

int main(int argc, char **argv) {
    BuildPaths paths;
    int pack = 1, validate = 1, publish = 1, rebuild_runtime = 0, packaged = 0, i;
    double total_begin, bootstrap = 0, candidate_runtime = 0, pack_begin, pack_seconds = 0,
           validate_begin, validate_seconds = 0, publish_begin, publish_seconds = 0;
    initialize_paths(&paths);
    for (i = 1; i < argc; ++i) {
        if (!_stricmp(argv[i], "-NoPack")) pack = 0;
        else if (!_stricmp(argv[i], "-RebuildRuntime")) rebuild_runtime = 1;
        else if (!_stricmp(argv[i], "-Map")) build_map = 1;
        else if (!_stricmp(argv[i], "-NoValidate")) validate = 0;
        else if (!_stricmp(argv[i], "-NoPublish")) publish = 0;
        else if (!_stricmp(argv[i], "-Help") || !_stricmp(argv[i], "--help")) { usage(); return 0; }
        else { usage(); nt_die("unknown build option", argv[i]); }
    }
    if (!pack || !validate) publish = 0;
    if (!nt_exists(paths.cpc)) nt_die("root cpc.exe missing", paths.cpc);
    nt_mkdirs(paths.build); nt_mkdirs(paths.compiler); nt_mkdirs(paths.bootstrap);
    total_begin = nt_seconds();
    printf("Building CPC with %s\n", paths.cpc);
    if (!build_compiler(&paths, &bootstrap)) return 1;
    if (pack) {
        pack_begin = nt_seconds();
        if (!rebuild_runtime) packaged = package_cached(&paths);
        pack_seconds = nt_seconds() - pack_begin;
    }
    if (rebuild_runtime || (pack && !packaged)) {
        if (!build_candidate_runtime(&paths, &candidate_runtime)) return 1;
        if (pack) {
            pack_begin = nt_seconds();
            if (!prepare_package(&paths)) return 1;
            pack_seconds += nt_seconds() - pack_begin;
        }
    }
    if (validate) {
        validate_begin = nt_seconds();
        if (!validate_candidate(&paths)) return 1;
        validate_seconds = nt_seconds() - validate_begin;
    }
    if (publish) {
        publish_begin = nt_seconds();
        if (!publish_candidate(&paths)) return 1;
        publish_seconds = nt_seconds() - publish_begin;
        printf("Success: rebuilt and replaced %s\n", paths.cpc);
    } else printf("Success: candidate retained at %s\n", paths.candidate);
    printf("Timing: compiler %.3fs; packaged-runtime %.3fs; pack %.3fs; regression %.3fs; publish %.3fs; total %.3fs\n",
           bootstrap, candidate_runtime, pack_seconds, validate_seconds, publish_seconds,
           nt_seconds() - total_begin);
    return 0;
}
