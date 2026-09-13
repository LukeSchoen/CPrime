#include "../../scripts/common/native_tool.h"

int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--child")) {
        puts("path child started");
        return 0;
    }
    {
        char module[NT_PATH], directory[NT_PATH], old_path[32768], path[32768];
        const char *command[] = {"test-process-path.exe", "--child", NULL};
        NtProcessResult result;
        DWORD old_length;
        nt_module_directory(module, sizeof module);
        strcpy(directory, module); nt_parent(directory);
        old_length = GetEnvironmentVariableA("PATH", old_path, sizeof old_path);
        if (!old_length || old_length >= sizeof old_path) return 2;
        if (snprintf(path, sizeof path, "%s;%s", directory, old_path) >= (int)sizeof path) return 3;
        if (!SetEnvironmentVariableA("PATH", path)) return 4;
        result = nt_run(command, directory, 5000, 0);
        if (!result.started || result.timed_out || result.exit_code ||
            strcmp(result.output, "path child started\r\n")) {
            fprintf(stderr, "PATH child failed: started=%d timeout=%d exit=%lu error=%lu output=%s",
                    result.started, result.timed_out, (unsigned long)result.exit_code,
                    (unsigned long)result.system_error, result.output);
            nt_process_free(&result);
            return 1;
        }
        nt_process_free(&result);
    }
    puts("PASS native process PATH lookup");
    return 0;
}
