#include "../../scripts/common/native_tool.h"

static void usage(void) {
    puts("test-msvc.exe -RunExternal [-CompilerPath CPC] [-NativeCompilerPath CLANG]\n"
         "Runs explicitly authorized cross-compiler ABI checks. Without -RunExternal no external compiler is started.");
}

int main(int argc, char **argv) {
    char tests[NT_PATH], root[NT_PATH], cpc[NT_PATH], native[NT_PATH];
    int authorized = 0, i;
    nt_module_directory(tests, sizeof tests); strcpy(root, tests); nt_parent(root);
    nt_join(cpc, sizeof cpc, root, "cpc.exe");
    nt_join(native, sizeof native, root, "third-party\\clang\\bin\\clang.exe");
    for (i = 1; i < argc; ++i) {
        if (!_stricmp(argv[i], "-RunExternal")) authorized = 1;
        else if (!_stricmp(argv[i], "-CompilerPath") && i + 1 < argc) strcpy(cpc, argv[++i]);
        else if (!_stricmp(argv[i], "-NativeCompilerPath") && i + 1 < argc) strcpy(native, argv[++i]);
        else if (!_stricmp(argv[i], "-Help") || !_stricmp(argv[i], "--help")) { usage(); return 0; }
        else { usage(); return 2; }
    }
    if (!authorized) { fprintf(stderr, "External MSVC-compatible ABI checks require -RunExternal.\n"); return 2; }
    if (!nt_exists(cpc)) nt_die("CPC compiler not found", cpc);
    if (!nt_exists(native)) nt_die("native compiler not found", native);
    {
        static const char *names[] = {"msvc_layout", "msvc_members", "msvc_nullptr", "msvc_stack_probe", "msvc_record_return"};
        char temp_root[NT_PATH], work[NT_PATH], source[NT_PATH], object[NT_PATH], consumer[NT_PATH], exe[NT_PATH];
        int failed = 0, k;
        GetTempPathA(sizeof temp_root, temp_root);
        snprintf(work, sizeof work, "%scprime-native-abi-%lu", temp_root, (unsigned long)GetCurrentProcessId());
        if (nt_exists(work)) nt_remove_tree(work); nt_mkdirs(work);
        for (k = 0; k < (int)(sizeof names / sizeof names[0]); ++k) {
            const char *native_args[20], *cpc_args[12], *run_args[2];
            NtProcessResult result; int n = 0;
            snprintf(source, sizeof source, "%s\\Tests\\abi\\%s\\provider.cpp", root, names[k]);
            snprintf(consumer, sizeof consumer, "%s\\Tests\\abi\\%s\\consumer.cpp", root, names[k]);
            snprintf(object, sizeof object, "%s\\%s-provider.obj", work, names[k]);
            snprintf(exe, sizeof exe, "%s\\%s.exe", work, names[k]);
            native_args[n++] = native; native_args[n++] = "--target=x86_64-pc-windows-msvc";
            native_args[n++] = "-std=c++17"; native_args[n++] = "-Werror"; native_args[n++] = "-fno-autolink";
            native_args[n++] = "-c"; native_args[n++] = source; native_args[n++] = "-o"; native_args[n++] = object; native_args[n] = NULL;
            result = nt_run(native_args, root, 120000, 1);
            if (!result.started || result.timed_out || result.exit_code) { fprintf(stderr,"FAIL %s provider\n",names[k]); failed++; nt_process_free(&result); continue; }
            nt_process_free(&result);
            cpc_args[0] = cpc; cpc_args[1] = "-Werror"; cpc_args[2] = consumer; cpc_args[3] = object;
            cpc_args[4] = "-o"; cpc_args[5] = exe; cpc_args[6] = NULL;
            result = nt_run(cpc_args, root, 120000, 1);
            if (!result.started || result.timed_out || result.exit_code) { fprintf(stderr,"FAIL %s mixed link\n",names[k]); failed++; nt_process_free(&result); continue; }
            nt_process_free(&result); run_args[0] = exe; run_args[1] = NULL;
            result = nt_run(run_args, work, 30000, 1);
            if (!result.started || result.timed_out || result.exit_code) { fprintf(stderr,"FAIL %s runtime\n",names[k]); failed++; }
            else printf("PASS %s\n", names[k]);
            nt_process_free(&result);
        }
        nt_remove_tree(work);
        return failed ? 1 : 0;
    }
}
