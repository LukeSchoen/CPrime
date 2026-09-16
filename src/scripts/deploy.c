#include "common/native_tool.h"

static int find_on_path(const char *name, char *out, size_t capacity) {
    DWORD length = SearchPathA(NULL, name, ".exe", (DWORD)capacity, out, NULL);
    return length && length < capacity;
}

int main(int argc, char **argv) {
    char scripts[NT_PATH], root[NT_PATH], deploy[NT_PATH], bin[NT_PATH], include[NT_PATH],
         include_cprime[NT_PATH], lib[NT_PATH], cpc[NT_PATH], header[NT_PATH], target_header[NT_PATH],
         dll[NT_PATH], def[NT_PATH], implib[NT_PATH], flag[NT_PATH + 4], source[NT_PATH], tool[NT_PATH],
         cache[NT_PATH], packager[NT_PATH], payload[NT_PATH], runtime_lib[NT_PATH], staged[NT_PATH];
    const char *compile[20], *impdef[6], *library[10], *package[7];
    NtProcessResult result;
    int n = 0, made_implib = 0;
    (void)argc; (void)argv;
    nt_module_directory(scripts, sizeof scripts); nt_tree_root(root, sizeof root);
    nt_join(cpc, sizeof cpc, root, "cpc.exe");
    nt_join(header, sizeof header, root, "src\\include\\cprime\\libcprime.h");
    if (!nt_exists(cpc)) nt_die("missing compiler", cpc);
    if (!nt_exists(header)) nt_die("missing public header", header);
    nt_join(deploy, sizeof deploy, root, "src\\deploy"); nt_join(bin, sizeof bin, deploy, "bin");
    nt_join(include, sizeof include, deploy, "include"); nt_join(include_cprime, sizeof include_cprime, include, "cprime");
    nt_join(lib, sizeof lib, deploy, "lib"); nt_mkdirs(bin); nt_mkdirs(include_cprime); nt_mkdirs(lib);
    nt_join(target_header, sizeof target_header, include_cprime, "libcprime.h"); nt_copy_file(header, target_header);
    nt_join(dll, sizeof dll, bin, "libcprime.dll"); nt_join(def, sizeof def, lib, "libcprime.def");
    nt_join(implib, sizeof implib, lib, "libcprime.lib");
    /* Build through a staging path in the build tree: a failed package step
       must leave the previously deployed runtime in place, and the import
       definition step writes a sidecar next to whatever DLL it is given. */
    nt_join(staged, sizeof staged, root, "src\\build\\deploy\\libcprime.dll");
    { char staging[NT_PATH]; nt_join(staging, sizeof staging, root, "src\\build\\deploy"); nt_mkdirs(staging); }
    DeleteFileA(staged);
    compile[n++] = cpc;
#define ADD_INCLUDE(relative) do { nt_join(source, sizeof source, root, relative); snprintf(flag, sizeof flag, "-I%s", source); compile[n++] = nt_strdup(flag); } while (0)
    ADD_INCLUDE("src\\include\\runtime"); ADD_INCLUDE("src\\include\\cprime");
    ADD_INCLUDE("src\\third-party\\win32-sdk\\include"); ADD_INCLUDE("src\\third-party\\win32-sdk\\include\\winapi");
    ADD_INCLUDE("src\\compiler\\frontend"); ADD_INCLUDE("src\\compiler\\middleend"); ADD_INCLUDE("src\\compiler\\backend\\x64");
    snprintf(flag, sizeof flag, "-I%s", root); compile[n++] = nt_strdup(flag);
    compile[n++] = "-DCPRIME_TARGET_PE"; compile[n++] = "-DCPRIME_TARGET_X86_64";
    compile[n++] = "-DLIBCPRIME_AS_DLL"; compile[n++] = "-shared";
    nt_join(source, sizeof source, root, "src\\compiler\\middleend\\libcprime.c"); compile[n++] = source;
    compile[n++] = "-o"; compile[n++] = staged; compile[n] = NULL;
    result = nt_run(compile, root, 300000, 1);
    if (!result.started || result.timed_out || result.exit_code) nt_die("libcprime DLL build failed", result.output);
    nt_process_free(&result);
    impdef[0] = cpc; impdef[1] = "-impdef"; impdef[2] = staged; impdef[3] = "-o"; impdef[4] = def; impdef[5] = NULL;
    result = nt_run(impdef, root, 30000, 1);
    if (!result.started || result.timed_out || result.exit_code) nt_die("import definition generation failed", result.output);
    nt_process_free(&result);
    /* The deployed runtime carries the compiler's own include/lib set, which
       libcprime unpacks on first use.  Reuse the payload the compiler build
       prepared and validated rather than shipping the runtime as loose files. */
    nt_join(cache, sizeof cache, root, "src\\build\\portable-cache");
    nt_join(packager, sizeof packager, cache, "portable-payload.exe");
    nt_join(payload, sizeof payload, cache, "payload.bin");
    nt_join(runtime_lib, sizeof runtime_lib, root, "src\\build\\compiler\\lib");
    if (!nt_exists(packager) || !nt_exists(payload))
        nt_die("portable payload is not prepared; run src\\scripts\\build.exe first", cache);
    package[0] = packager; package[1] = "cached"; package[2] = staged; package[3] = cache;
    package[4] = root; package[5] = runtime_lib; package[6] = NULL;
    result = nt_run(package, root, 120000, 1);
    if (!result.started || result.timed_out || result.exit_code) {
        /* The cached payload is a prepared runtime; a runtime source edit can
           make it stale before the next compiler build refreshes it.  Attach it
           anyway and say so, rather than shipping a DLL with no runtime. */
        nt_process_free(&result);
        package[1] = "attach";
        package[3] = payload;   /* attach EXE PAYLOAD */
        package[4] = NULL;
        result = nt_run(package, root, 120000, 1);
        if (!result.started || result.timed_out || result.exit_code) {
            DeleteFileA(staged);
            nt_die("portable payload attach failed", result.output);
        }
        printf("WARN: cached portable payload is stale; run src\\scripts\\build.exe to refresh it\n");
    }
    nt_process_free(&result);
    if (!MoveFileExA(staged, dll, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(staged);
        nt_die("could not publish the deployed runtime", dll);
    }
    if (find_on_path("lib", tool, sizeof tool) || find_on_path("llvm-lib", tool, sizeof tool)) {
        char defarg[NT_PATH + 8], outarg[NT_PATH + 8];
        snprintf(defarg, sizeof defarg, "/def:%s", def); snprintf(outarg, sizeof outarg, "/out:%s", implib);
        library[0] = tool; library[1] = "/nologo"; library[2] = defarg;
        library[3] = outarg; library[4] = "/machine:x64"; library[5] = NULL;
        result = nt_run(library, root, 30000, 1);
        made_implib = result.started && !result.timed_out && result.exit_code == 0;
        nt_process_free(&result);
    }
    printf("Deployed libcprime to %s\n  bin\\libcprime.dll\n  include\\cprime\\libcprime.h\n  lib\\libcprime.def\n", deploy);
    if (made_implib) puts("  lib\\libcprime.lib");
    else puts("WARN: no Microsoft-compatible library tool found; skipped lib\\libcprime.lib");
    return 0;
}
