#ifndef CPRIME_NATIVE_TOOL_H
#define CPRIME_NATIVE_TOOL_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NT_PATH 4096
#define NT_ARGS 512

typedef struct NtBuffer {
    char *data;
    size_t size;
    size_t capacity;
} NtBuffer;

typedef struct NtProcessResult {
    DWORD exit_code;
    int started;
    int timed_out;
    double wall_seconds;
    double cpu_seconds;
    char *output;
} NtProcessResult;

static void nt_die(const char *message, const char *detail) {
    fprintf(stderr, "ERROR: %s%s%s\n", message, detail && *detail ? ": " : "",
            detail && *detail ? detail : "");
    exit(1);
}

static void *nt_alloc(size_t size) {
    void *p = malloc(size ? size : 1);
    if (!p) nt_die("out of memory", "");
    return p;
}

static char *nt_strdup(const char *text) {
    size_t size = strlen(text) + 1;
    char *copy = nt_alloc(size);
    memcpy(copy, text, size);
    return copy;
}

static void nt_buffer_append(NtBuffer *buffer, const void *data, size_t size) {
    size_t capacity;
    char *next;
    if (size > SIZE_MAX - buffer->size - 1) nt_die("buffer too large", "");
    if (buffer->size + size + 1 > buffer->capacity) {
        capacity = buffer->capacity ? buffer->capacity : 256;
        while (capacity < buffer->size + size + 1) {
            if (capacity > SIZE_MAX / 2) nt_die("buffer too large", "");
            capacity *= 2;
        }
        next = realloc(buffer->data, capacity);
        if (!next) nt_die("out of memory", "");
        buffer->data = next;
        buffer->capacity = capacity;
    }
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
    buffer->data[buffer->size] = 0;
}

static void nt_buffer_text(NtBuffer *buffer, const char *text) {
    nt_buffer_append(buffer, text, strlen(text));
}

static void nt_buffer_free(NtBuffer *buffer) {
    free(buffer->data);
    memset(buffer, 0, sizeof *buffer);
}

static int nt_exists(const char *path) {
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES;
}

static int nt_is_directory(const char *path) {
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static void nt_join(char *out, size_t capacity, const char *left, const char *right) {
    size_t n = strlen(left), m = strlen(right);
    int slash = n && left[n - 1] != '\\' && left[n - 1] != '/';
    while (*right == '\\' || *right == '/') { right++; m--; }
    if (n + slash + m + 1 > capacity) nt_die("path too long", left);
    memcpy(out, left, n);
    if (slash) out[n++] = '\\';
    memcpy(out + n, right, m + 1);
}

static void nt_parent(char *path) {
    char *slash = strrchr(path, '\\');
    char *other = strrchr(path, '/');
    if (!slash || other > slash) slash = other;
    if (!slash) nt_die("path has no parent", path);
    while (slash > path && (slash[-1] == '\\' || slash[-1] == '/')) slash--;
    *slash = 0;
}

static void nt_module_directory(char *out, size_t capacity) {
    DWORD length = GetModuleFileNameA(NULL, out, (DWORD)capacity);
    if (!length || length >= capacity) nt_die("cannot locate executable", "");
    nt_parent(out);
}

static void nt_mkdirs(const char *path) {
    char copy[NT_PATH];
    char *p;
    if (strlen(path) >= sizeof copy) nt_die("path too long", path);
    strcpy(copy, path);
    for (p = copy + 3; *p; ++p) {
        if (*p == '\\' || *p == '/') {
            char saved = *p;
            *p = 0;
            if (!CreateDirectoryA(copy, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
                nt_die("cannot create directory", copy);
            *p = saved;
        }
    }
    if (!CreateDirectoryA(copy, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        nt_die("cannot create directory", copy);
}

static unsigned char *nt_read_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    long length;
    unsigned char *data;
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) || (length = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET)) nt_die("cannot seek file", path);
    data = nt_alloc((size_t)length + 1);
    if (fread(data, 1, (size_t)length, file) != (size_t)length)
        nt_die("cannot read file", path);
    fclose(file);
    data[length] = 0;
    *size = (size_t)length;
    return data;
}

static void nt_write_file(const char *path, const void *data, size_t size) {
    FILE *file;
    char directory[NT_PATH];
    if (strlen(path) >= sizeof directory) nt_die("path too long", path);
    strcpy(directory, path);
    nt_parent(directory);
    nt_mkdirs(directory);
    file = fopen(path, "wb");
    if (!file) nt_die("cannot create file", path);
    if (fwrite(data, 1, size, file) != size || fclose(file))
        nt_die("cannot write file", path);
}

static void nt_copy_file(const char *source, const char *destination) {
    char directory[NT_PATH];
    if (strlen(destination) >= sizeof directory) nt_die("path too long", destination);
    strcpy(directory, destination);
    nt_parent(directory);
    nt_mkdirs(directory);
    if (!CopyFileA(source, destination, FALSE)) nt_die("cannot copy file", source);
}

static void nt_copy_tree(const char *source, const char *destination) {
    WIN32_FIND_DATAA entry;
    HANDLE find;
    char pattern[NT_PATH], from[NT_PATH], to[NT_PATH];
    if (!nt_is_directory(source)) nt_die("source directory not found", source);
    nt_mkdirs(destination);
    nt_join(pattern, sizeof pattern, source, "*");
    find = FindFirstFileA(pattern, &entry);
    if (find == INVALID_HANDLE_VALUE) nt_die("cannot enumerate directory", source);
    do {
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        nt_join(from, sizeof from, source, entry.cFileName);
        nt_join(to, sizeof to, destination, entry.cFileName);
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            nt_die("refusing redirected tree entry", from);
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) nt_copy_tree(from, to);
        else nt_copy_file(from, to);
    } while (FindNextFileA(find, &entry));
    if (GetLastError() != ERROR_NO_MORE_FILES) nt_die("cannot enumerate directory", source);
    FindClose(find);
}

static void nt_remove_tree(const char *path) {
    WIN32_FIND_DATAA entry;
    HANDLE find;
    char pattern[NT_PATH], child[NT_PATH];
    DWORD attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) return;
    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) nt_die("refusing redirected cleanup", path);
    if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
        if (!DeleteFileA(path)) nt_die("cannot remove file", path);
        return;
    }
    nt_join(pattern, sizeof pattern, path, "*");
    find = FindFirstFileA(pattern, &entry);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
            nt_join(child, sizeof child, path, entry.cFileName);
            nt_remove_tree(child);
        } while (FindNextFileA(find, &entry));
        FindClose(find);
    }
    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    if (!RemoveDirectoryA(path)) nt_die("cannot remove directory", path);
}

static double nt_seconds(void) {
    LARGE_INTEGER value, frequency;
    QueryPerformanceCounter(&value);
    QueryPerformanceFrequency(&frequency);
    return (double)value.QuadPart / (double)frequency.QuadPart;
}

static void nt_quote(NtBuffer *command, const char *argument) {
    const char *p;
    unsigned slashes = 0;
    nt_buffer_text(command, "\"");
    for (p = argument; ; ++p) {
        if (*p == '\\') { slashes++; continue; }
        if (*p == '"') {
            while (slashes--) nt_buffer_text(command, "\\\\");
            nt_buffer_text(command, "\\\"");
            slashes = 0;
            continue;
        }
        while (slashes) { nt_buffer_text(command, "\\"); slashes--; }
        if (!*p) break;
        nt_buffer_append(command, p, 1);
    }
    nt_buffer_text(command, "\"");
}

static NtProcessResult nt_run(const char *const *arguments, const char *cwd,
                              unsigned timeout_ms, int echo_output) {
    NtProcessResult result;
    SECURITY_ATTRIBUTES security = {sizeof security, NULL, TRUE};
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;
    HANDLE output_read = NULL, output_write = NULL, job = NULL;
    NtBuffer command = {0}, captured = {0};
    char chunk[4096];
    DWORD available, read, wait;
    double begin = nt_seconds();
    int i;
    memset(&result, 0, sizeof result);
    result.exit_code = (DWORD)-1;
    memset(&startup, 0, sizeof startup);
    memset(&process, 0, sizeof process);
    memset(&limits, 0, sizeof limits);
    startup.cb = sizeof startup;
    for (i = 0; arguments[i]; ++i) {
        if (i) nt_buffer_text(&command, " ");
        nt_quote(&command, arguments[i]);
    }
    if (!CreatePipe(&output_read, &output_write, &security, 0)) goto cleanup;
    SetHandleInformation(output_read, HANDLE_FLAG_INHERIT, 0);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = output_write;
    startup.hStdError = output_write;
    job = CreateJobObjectA(NULL, NULL);
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (job) SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                     &limits, sizeof limits);
    if (!CreateProcessA(arguments[0], command.data, NULL, NULL, TRUE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED, NULL, cwd,
                        &startup, &process)) goto cleanup;
    result.started = 1;
    if (job) AssignProcessToJobObject(job, process.hProcess);
    ResumeThread(process.hThread);
    CloseHandle(output_write);
    output_write = NULL;
    for (;;) {
        while (PeekNamedPipe(output_read, NULL, 0, NULL, &available, NULL) && available) {
            DWORD amount = available < sizeof chunk ? available : sizeof chunk;
            if (!ReadFile(output_read, chunk, amount, &read, NULL) || !read) break;
            nt_buffer_append(&captured, chunk, read);
            if (echo_output) fwrite(chunk, 1, read, stdout);
        }
        wait = WaitForSingleObject(process.hProcess, 1);
        if (wait == WAIT_OBJECT_0) break;
        if (timeout_ms && (nt_seconds() - begin) * 1000.0 >= timeout_ms) {
            result.timed_out = 1;
            if (job) TerminateJobObject(job, 0xffffffffu);
            else TerminateProcess(process.hProcess, 0xffffffffu);
            WaitForSingleObject(process.hProcess, 1000);
            break;
        }
    }
    while (ReadFile(output_read, chunk, sizeof chunk, &read, NULL) && read) {
        nt_buffer_append(&captured, chunk, read);
        if (echo_output) fwrite(chunk, 1, read, stdout);
    }
    if (!result.timed_out) GetExitCodeProcess(process.hProcess, &result.exit_code);
    {
        FILETIME create, exit, kernel, user;
        ULARGE_INTEGER k, u;
        if (GetProcessTimes(process.hProcess, &create, &exit, &kernel, &user)) {
            k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
            u.LowPart = user.dwLowDateTime; u.HighPart = user.dwHighDateTime;
            result.cpu_seconds = (double)(k.QuadPart + u.QuadPart) / 10000000.0;
        }
    }
cleanup:
    result.wall_seconds = nt_seconds() - begin;
    result.output = captured.data ? captured.data : nt_strdup("");
    if (output_write) CloseHandle(output_write);
    if (output_read) CloseHandle(output_read);
    if (process.hThread) CloseHandle(process.hThread);
    if (process.hProcess) CloseHandle(process.hProcess);
    if (job) CloseHandle(job);
    nt_buffer_free(&command);
    return result;
}

static void nt_process_free(NtProcessResult *result) {
    free(result->output);
    memset(result, 0, sizeof *result);
}

#endif
