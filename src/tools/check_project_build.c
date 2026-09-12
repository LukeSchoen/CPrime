/* Validate the complete build before skipping the native project driver. A missing
   final executable alone can be relinked; all other changes use the driver. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int read_u32(FILE *f, int *ok)
{
    unsigned int v = 0;
    if (fread(&v, sizeof v, 1, f) != 1) *ok = 0;
    return v;
}

static unsigned long long read_u64(FILE *f, int *ok)
{
    unsigned long long v = 0;
    if (fread(&v, sizeof v, 1, f) != 1) *ok = 0;
    return v;
}

static wchar_t *read_string(FILE *f, int *ok)
{
    unsigned int n = read_u32(f, ok);
    char *utf8;
    wchar_t *wide;
    int length;
    if (!*ok || n > 1024 * 1024) { *ok = 0; return NULL; }
    utf8 = (char *)malloc(n + 1);
    if (!utf8) { *ok = 0; return NULL; }
    if (fread(utf8, 1, n, f) != n) { free(utf8); *ok = 0; return NULL; }
    if (memchr(utf8, 0, n)) { free(utf8); *ok = 0; return NULL; }
    utf8[n] = 0;
    length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, n + 1, NULL, 0);
    wide = length ? (wchar_t *)malloc(length * sizeof(wchar_t)) : NULL;
    if (!wide || !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, n + 1, wide, length)) {
        free(wide); wide = NULL; *ok = 0;
    }
    free(utf8);
    return wide;
}

int main(int argc, char **argv)
{
    FILE *f;
    char magic[8];
    unsigned int units, count, i;
    unsigned int timeout;
    int ok = 1, missing_output = 0, output_records = 0;
    long output_position = 0;
    wchar_t *output, *compiler, *command, *directory;
    LARGE_INTEGER begin, end, frequency;
    if (argc != 2 && (argc != 3 || strcmp(argv[2], "--check-only"))) return 1;
    QueryPerformanceCounter(&begin);
    QueryPerformanceFrequency(&frequency);
    f = fopen(argv[1], "rb");
    if (!f) return 1;
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "CPCCHK02", 8)) { fclose(f); return 1; }
    output = read_string(f, &ok);
    compiler = read_string(f, &ok);
    command = read_string(f, &ok);
    directory = read_string(f, &ok);
    timeout = read_u32(f, &ok);
    if (!timeout || timeout > 3600) ok = 0;
    units = read_u32(f, &ok);
    count = read_u32(f, &ok);
    if (count > 1024) ok = 0;
    for (i = 0; ok && i < count; ++i) {
        wchar_t *name = read_string(f, &ok), *expected = read_string(f, &ok);
        wchar_t *actual = NULL;
        DWORD size;
        if (ok) {
            size = GetEnvironmentVariableW(name, NULL, 0);
            actual = (wchar_t *)calloc(size + 1, sizeof(wchar_t));
            if (!actual) ok = 0;
            else {
                if (size && !GetEnvironmentVariableW(name, actual, size)) ok = 0;
                if (lstrcmpW(actual, expected)) ok = 0;
            }
        }
        free(name); free(expected); free(actual);
    }
    count = read_u32(f, &ok);
    if (!count || count > 1000000) ok = 0;
    for (i = 0; ok && i < count; ++i) {
        wchar_t *path = read_string(f, &ok);
        unsigned int kind = read_u32(f, &ok);
        long position = ftell(f);
        unsigned long long time = read_u64(f, &ok), size = read_u64(f, &ok);
        WIN32_FILE_ATTRIBUTE_DATA info;
        if (ok) {
            if (kind > 3) ok = 0;
            if (kind == 3) {
                ++output_records;
                output_position = position;
                if (lstrcmpW(path, output)) ok = 0;
            }
        }
        if (ok) {
            if (!GetFileAttributesExW(path, GetFileExInfoStandard, &info)) {
                DWORD error = GetLastError();
                ok = (kind == 2 || (kind == 3 && argc == 2))
                    && (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND);
                if (ok && kind == 3) missing_output = 1;
            } else {
                unsigned long long actual_time = ((unsigned long long)info.ftLastWriteTime.dwHighDateTime << 32) | info.ftLastWriteTime.dwLowDateTime;
                unsigned long long actual_size = ((unsigned long long)info.nFileSizeHigh << 32) | info.nFileSizeLow;
                ok = kind != 2 && !!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == (kind == 1)
                    && time == actual_time && (kind == 1 || size == actual_size);
            }
        }
        free(path);
    }
    if (ok && fgetc(f) != EOF) ok = 0;
    fclose(f);
    if (output_records != 1) ok = 0;
    if (!ok) return 1;
    if (missing_output) {
        STARTUPINFOW startup;
        PROCESS_INFORMATION process;
        WIN32_FILE_ATTRIBUTE_DATA info;
        DWORD result = 1;
        unsigned long long stamp, size;
        /* Invalidate before starting: failure or interruption cannot leave a
           partially written executable certified by the previous snapshot. */
        f = fopen(argv[1], "r+b");
        if (!f) return 1;
        if (fwrite("INVALID!", 1, 8, f) != 8 || fflush(f)) { fclose(f); return 1; }
        memset(&startup, 0, sizeof startup);
        memset(&process, 0, sizeof process);
        startup.cb = sizeof startup;
        if (CreateProcessW(compiler, command, NULL, NULL, TRUE, 0, NULL, directory, &startup, &process)) {
            if (WaitForSingleObject(process.hProcess, timeout * 1000) != WAIT_OBJECT_0) {
                TerminateProcess(process.hProcess, 1);
                WaitForSingleObject(process.hProcess, INFINITE);
            } else GetExitCodeProcess(process.hProcess, &result);
            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);
        }
        if (result || !GetFileAttributesExW(output, GetFileExInfoStandard, &info)
            || (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            DeleteFileW(output);
            fclose(f);
            return 1;
        }
        stamp = ((unsigned long long)info.ftLastWriteTime.dwHighDateTime << 32) | info.ftLastWriteTime.dwLowDateTime;
        size = ((unsigned long long)info.nFileSizeHigh << 32) | info.nFileSizeLow;
        /* Refresh only the native certificate. The driver's old JSON output
           stamp remains conservative: a later full-driver build may relink
           once more, but cannot mistake this output for an unchecked build. */
        ok = !fseek(f, output_position, SEEK_SET)
            && fwrite(&stamp, sizeof stamp, 1, f) == 1 && fwrite(&size, sizeof size, 1, f) == 1
            && !fflush(f) && !fseek(f, 0, SEEK_SET) && fwrite("CPCCHK02", 1, 8, f) == 8;
        if (fclose(f)) ok = 0;
        if (!ok) return 1;
        QueryPerformanceCounter(&end);
        printf("Linked (%u units, %llums elapsed)\n", units,
            (unsigned long long)((double)(end.QuadPart - begin.QuadPart) * 1000.0 / frequency.QuadPart + 0.5));
        return 0;
    }
    QueryPerformanceCounter(&end);
    printf("Up to date (%u units, %llums elapsed)\n",
           units, (unsigned long long)((double)(end.QuadPart - begin.QuadPart) * 1000.0 / frequency.QuadPart + 0.5));
    return 0;
}
