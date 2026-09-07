/* Native portable-package helper. Built serially by CPC on every full build. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void fail(const char *operation, const char *path) {
    fprintf(stderr, "portable payload: %s: %s (Windows error %lu)\n",
            operation, path, (unsigned long)GetLastError());
    exit(1);
}

static void *allocate(size_t size) {
    void *p = malloc(size ? size : 1);
    if (!p) fail("out of memory", "");
    return p;
}

static unsigned char *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    long length;
    unsigned char *data;
    if (!f) fail("open", path);
    if (fseek(f, 0, SEEK_END) || (length = ftell(f)) < 0 || fseek(f, 0, SEEK_SET))
        fail("seek", path);
    *size = (size_t)length;
    data = allocate(*size + 1);
    if (fread(data, 1, *size, f) != *size) fail("read", path);
    if (fclose(f)) fail("close", path);
    data[*size] = 0;
    return data;
}

static void write_file(const char *path, const void *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) fail("create", path);
    if (fwrite(data, 1, size, f) != size) fail("write", path);
    if (fclose(f)) fail("close", path);
}

/* Match the previous .NET whitespace normalization and ASCII encoding. */
static int whitespace(unsigned c) {
    return (c >= 9 && c <= 13) || c == 32 || c == 0x85 || c == 0xa0 ||
           c == 0x1680 || (c >= 0x2000 && c <= 0x200a) || c == 0x2028 ||
           c == 0x2029 || c == 0x202f || c == 0x205f || c == 0x3000;
}

static int punctuation(unsigned c) {
    return c && c < 128 && strchr(",;:{}()[]=+-*/%&|^!?<>~", (int)c) != NULL;
}

static void minify(const char *path) {
    size_t bytes, n, i, count = 0, used = 0;
    unsigned char *data = read_file(path, &bytes);
    wchar_t *text;
    char *output;
    int state = 0;
    if (bytes >= 4 && (!memcmp(data, "\xff\xfe\0\0", 4) ||
                       !memcmp(data, "\0\0\xfe\xff", 4))) {
        size_t units = (bytes - 4) / 4;
        text = allocate((units * 2 + 1) * sizeof(wchar_t));
        n = 0;
        for (i = 0; i < units; ++i) {
            unsigned char *p = data + 4 + i * 4;
            uint32_t c = data[0] == 255
                ? (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24)
                : ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
            if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) c = 0xfffd;
            if (c > 0xffff) {
                c -= 0x10000;
                text[n++] = 0xd800 + (c >> 10);
                text[n++] = 0xdc00 + (c & 1023);
            } else text[n++] = c;
        }
    } else if (bytes >= 2 && ((data[0] == 255 && data[1] == 254) ||
                      (data[0] == 254 && data[1] == 255))) {
        n = (bytes - 2) / 2;
        text = allocate((n + 1) * sizeof(wchar_t));
        for (i = 0; i < n; ++i)
            text[i] = data[0] == 255 ? data[2+i*2] | (data[3+i*2] << 8)
                                    : (data[2+i*2] << 8) | data[3+i*2];
    } else {
        size_t bom = bytes >= 3 && !memcmp(data, "\xef\xbb\xbf", 3) ? 3 : 0;
        int length;
        if (bytes > INT32_MAX) fail("header too large", path);
        length = MultiByteToWideChar(CP_UTF8, 0, (char *)data + bom,
                                    (int)(bytes - bom), NULL, 0);
        if (!length && bytes != bom) fail("decode UTF-8", path);
        n = length;
        text = allocate((n + 1) * sizeof(wchar_t));
        if (n && !MultiByteToWideChar(CP_UTF8, 0, (char *)data + bom,
                                      (int)(bytes - bom), text, length))
            fail("decode UTF-8", path);
    }
    free(data);
    /* Strip comments in place, retaining line breaks and quoted characters. */
    for (i = 0; i < n; ++i) {
        unsigned c = text[i], next = i + 1 < n ? text[i+1] : 0;
        if (!state) {
            if (c == '/' && (next == '/' || next == '*')) {
                text[count++] = ' ';
                state = next == '/' ? 3 : 4;
                ++i;
            } else {
                text[count++] = c;
                if (c == '"') state = 1;
                else if (c == '\'') state = 2;
            }
        } else if (state == 1 || state == 2) {
            text[count++] = c;
            if (c == '\\' && i + 1 < n) text[count++] = text[++i];
            else if ((state == 1 && c == '"') || (state == 2 && c == '\'')) state = 0;
        } else if (state == 3) {
            if (c == '\r' || c == '\n') { text[count++] = c; state = 0; }
        } else {
            if (c == '\r' || c == '\n') text[count++] = c;
            else if (c == '*' && next == '/') { text[count++] = ' '; ++i; state = 0; }
        }
    }
    output = allocate(count + 2);
    for (i = 0; i < count;) {
        size_t start = i, end;
        int directive;
        while (i < count && text[i] != '\n') ++i;
        end = i;
        if (i < count) ++i;
        while (start < end && whitespace(text[start])) ++start;
        while (end > start && whitespace(text[end-1])) --end;
        if (start == end) continue;
        directive = text[start] == '#';
        while (start < end) {
            unsigned c = text[start++];
            if (whitespace(c)) {
                unsigned previous = text[start-2];
                while (start < end && whitespace(text[start])) ++start;
                if (directive || (!punctuation(previous) && !punctuation(text[start])))
                    output[used++] = ' ';
            } else output[used++] = c < 128 ? (char)c : '?';
        }
        output[used++] = '\n';
    }
    if (!used) output[used++] = '\n';
    write_file(path, output, used);
    free(output);
    free(text);
}

static void join_path(char *out, size_t capacity, const char *base, const char *name) {
    if (strlen(base) + strlen(name) + 2 > capacity) fail("path too long", base);
    sprintf(out, "%s\\%s", base, name);
}

static void minify_directory(const char *path) {
    char child[4096];
    WIN32_FIND_DATAA entry;
    HANDLE find;
    join_path(child, sizeof(child), path, "*");
    find = FindFirstFileA(child, &entry);
    if (find == INVALID_HANDLE_VALUE) fail("enumerate directory", path);
    do {
        const char *ext;
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        join_path(child, sizeof(child), path, entry.cFileName);
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) minify_directory(child);
        else if ((ext = strrchr(entry.cFileName, '.')) && !_stricmp(ext, ".h")) minify(child);
    } while (FindNextFileA(find, &entry));
    if (GetLastError() != ERROR_NO_MORE_FILES) fail("enumerate directory", path);
    FindClose(find);
}

static unsigned char *compress_data(const unsigned char *source_data, size_t size, size_t *actual) {
    typedef BOOL (WINAPI *CreateFn)(DWORD, void *, void **);
    typedef BOOL (WINAPI *CompressFn)(void *, const void *, size_t, void *, size_t, size_t *);
    typedef BOOL (WINAPI *CloseFn)(void *);
    HMODULE library = LoadLibraryA("cabinet.dll");
    CreateFn create;
    CompressFn compress;
    CloseFn close;
    void *handle = NULL;
    size_t capacity;
    unsigned char *compressed;
    if (!library) fail("load compression API", "cabinet.dll");
    create = (CreateFn)GetProcAddress(library, "CreateCompressor");
    compress = (CompressFn)GetProcAddress(library, "Compress");
    close = (CloseFn)GetProcAddress(library, "CloseCompressor");
    if (!create || !compress || !close) fail("resolve compression API", "cabinet.dll");
    if (!create(5 /* LZMS */, NULL, &handle)) fail("create compressor", "");
    capacity = size + 65536;
    for (;;) {
        compressed = allocate(capacity);
        if (compress(handle, source_data, size, compressed, capacity, actual)) break;
        free(compressed);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || capacity > size*2 + 1048576)
            fail("compress", "");
        capacity *= 2;
    }
    close(handle);
    FreeLibrary(library);
    return compressed;
}

typedef struct Buffer { unsigned char *data; size_t size, capacity; } Buffer;

static void append(Buffer *buffer, const void *data, size_t size) {
    if (size > SIZE_MAX - buffer->size) fail("payload too large", "");
    if (buffer->size + size > buffer->capacity) {
        size_t capacity = buffer->size + size;
        unsigned char *next;
        if (capacity < SIZE_MAX / 2) capacity *= 2;
        next = realloc(buffer->data, capacity);
        if (!next) fail("out of memory", "");
        buffer->data = next;
        buffer->capacity = capacity;
    }
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
}

static void number(Buffer *buffer, uint64_t value, unsigned width) {
    unsigned char bytes[8];
    unsigned i;
    for (i = 0; i < width; ++i) { bytes[i] = value & 255; value >>= 8; }
    append(buffer, bytes, width);
}

static void pack(const char *exe, const char *stage, const char *manifest) {
    Buffer payload = {0}, trailer = {0};
    size_t length, i = 0, compressed_size;
    unsigned char *list = read_file(manifest, &length), *compressed;
    uint32_t count = 0;
    FILE *file;
    long offset;
    number(&payload, 0, 4);
    while (i < length) {
        size_t start = i, path_length, file_size;
        char path[4096];
        unsigned char *contents;
        while (i < length && list[i] != '\n') ++i;
        path_length = i - start;
        if (path_length && list[start + path_length - 1] == '\r') --path_length;
        if (i < length) ++i;
        if (!path_length) continue;
        list[start + path_length] = 0;
        if (path_length > 65535 || count == UINT32_MAX) fail("manifest too large", manifest);
        join_path(path, sizeof(path), stage, (char *)list + start);
        contents = read_file(path, &file_size);
        number(&payload, path_length, 2);
        number(&payload, file_size, 8);
        append(&payload, list + start, path_length);
        append(&payload, contents, file_size);
        free(contents);
        ++count;
    }
    for (i = 0; i < 4; ++i) payload.data[i] = (count >> (8*i)) & 255;
    compressed = compress_data(payload.data, payload.size, &compressed_size);
    file = fopen(exe, "ab");
    if (!file || fseek(file, 0, SEEK_END) || (offset = ftell(file)) < 0) fail("open executable", exe);
    number(&trailer, payload.size, 8);
    number(&trailer, compressed_size, 8);
    append(&trailer, compressed, compressed_size);
    append(&trailer, "CPCPAY11", 8);
    number(&trailer, (uint64_t)offset, 8);
    if (fwrite(trailer.data, 1, trailer.size, file) != trailer.size) fail("append payload", exe);
    if (fclose(file)) fail("close executable", exe);
    free(list);
    free(payload.data);
    free(trailer.data);
    free(compressed);
}

int main(int argc, char **argv) {
    if (argc == 3 && !strcmp(argv[1], "minify")) minify(argv[2]);
    else if (argc == 3 && !strcmp(argv[1], "headers")) minify_directory(argv[2]);
    else if (argc == 5 && !strcmp(argv[1], "pack")) pack(argv[2], argv[3], argv[4]);
    else {
        fprintf(stderr, "usage: portable-payload minify FILE | headers DIRECTORY | pack EXE STAGE MANIFEST\n");
        return 1;
    }
    return 0;
}
