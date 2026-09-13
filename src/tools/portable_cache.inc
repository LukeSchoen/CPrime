/* Content hashes deliberately ignore mtimes: rebuilding identical runtime
   objects must not cause SDK minification and compression on every build. */
typedef struct CacheHash {
    HMODULE dll;
    uintptr_t provider, hash;
    BOOL (WINAPI *acquire)(uintptr_t *, const char *, const char *, DWORD, DWORD);
    BOOL (WINAPI *create)(uintptr_t, unsigned, uintptr_t, DWORD, uintptr_t *);
    BOOL (WINAPI *update)(uintptr_t, const BYTE *, DWORD, DWORD);
    BOOL (WINAPI *get)(uintptr_t, DWORD, BYTE *, DWORD *, DWORD);
    BOOL (WINAPI *destroy)(uintptr_t);
    BOOL (WINAPI *release)(uintptr_t, DWORD);
} CacheHash;

static void hash_begin(CacheHash *h) {
    memset(h, 0, sizeof *h);
    h->dll = LoadLibraryA("advapi32.dll");
    if (!h->dll) fail("load hash provider", "advapi32.dll");
    h->acquire = (void *)GetProcAddress(h->dll, "CryptAcquireContextA");
    h->create = (void *)GetProcAddress(h->dll, "CryptCreateHash");
    h->update = (void *)GetProcAddress(h->dll, "CryptHashData");
    h->get = (void *)GetProcAddress(h->dll, "CryptGetHashParam");
    h->destroy = (void *)GetProcAddress(h->dll, "CryptDestroyHash");
    h->release = (void *)GetProcAddress(h->dll, "CryptReleaseContext");
    if (!h->acquire || !h->create || !h->update || !h->get || !h->destroy || !h->release)
        fail("resolve hash API", "advapi32.dll");
    /* PROV_RSA_AES, CRYPT_VERIFYCONTEXT, CALG_SHA_256. */
    if (!h->acquire(&h->provider, NULL, NULL, 24, 0xf0000000u) ||
        !h->create(h->provider, 0x800c, 0, 0, &h->hash)) fail("create SHA256", "");
}

static void hash_bytes(CacheHash *h, const void *bytes, size_t size) {
    if (size > UINT32_MAX || !h->update(h->hash, bytes, (DWORD)size, 0)) fail("hash data", "");
}

static void hash_end(CacheHash *h, unsigned char digest[32]) {
    DWORD size = 32;
    if (!h->get(h->hash, 2 /* HP_HASHVAL */, digest, &size, 0) || size != 32)
        fail("finish SHA256", "");
    h->destroy(h->hash);
    h->release(h->provider, 0);
    FreeLibrary(h->dll);
}

static int hash_file(CacheHash *h, const char *path) {
    unsigned char buffer[65536];
    FILE *file = fopen(path, "rb");
    size_t read;
    uint64_t size;
    long length;
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) || (length = ftell(file)) < 0 || fseek(file, 0, SEEK_SET)) {
        fclose(file); return 0;
    }
    size = (uint64_t)length;
    hash_bytes(h, path, strlen(path) + 1);
    hash_bytes(h, &size, sizeof size);
    while ((read = fread(buffer, 1, sizeof buffer, file)) != 0) hash_bytes(h, buffer, read);
    if (ferror(file)) { fclose(file); return 0; }
    fclose(file);
    return 1;
}

static int compare_paths(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static int hash_tree(CacheHash *h, const char *directory) {
    WIN32_FIND_DATAA entry;
    HANDLE find;
    char path[4096], **names = NULL;
    size_t count = 0, i;
    int ok = 1;
    join_path(path, sizeof path, directory, "*");
    find = FindFirstFileA(path, &entry);
    if (find == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND;
    do {
        char *name;
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        name = allocate(strlen(entry.cFileName) + 1);
        strcpy(name, entry.cFileName);
        names = realloc(names, (count + 1) * sizeof *names);
        if (!names) fail("allocate names", directory);
        names[count++] = name;
    } while (FindNextFileA(find, &entry));
    if (GetLastError() != ERROR_NO_MORE_FILES) ok = 0;
    FindClose(find);
    qsort(names, count, sizeof *names, compare_paths);
    hash_bytes(h, directory, strlen(directory) + 1);
    for (i = 0; i < count; ++i) {
        DWORD attributes;
        join_path(path, sizeof path, directory, names[i]);
        attributes = GetFileAttributesA(path);
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) ok = 0;
        else if (attributes & FILE_ATTRIBUTE_DIRECTORY) { if (!hash_tree(h, path)) ok = 0; }
        else if (!hash_file(h, path)) ok = 0;
        free(names[i]);
    }
    free(names);
    return ok;
}

static int cache_digest(const char *cache, const char *root, const char *runtime, unsigned char digest[32]) {
    static const char *trees[] = { "include/runtime", "src/runtime",
        "third-party/quickjs-regexp", "third-party/win32-sdk/include",
        "third-party/win32-sdk/lib" };
    static const char *files[] = { "src/tools/portable_payload.c", "src/tools/portable_cache.inc",
        "src/tools/check_runtime_abi.c", "src/tools/check_runtime_abi.S" };
    static const char *dlls[] = { "kernel32", "user32", "gdi32", "ws2_32", "msvcrt", "ucrtbase",
        "comdlg32", "shell32", "uxtheme", "dwmapi", "msimg32", "advapi32", "comctl32",
        "ole32", "oleaut32", "rpcrt4", "shlwapi", "windowscodecs", "imm32", "shcore" };
    CacheHash h;
    char path[4096], name[100], system[4096];
    unsigned i;
    int ok = 1;
    hash_begin(&h);
    hash_bytes(&h, "CPC prepared payload 3", sizeof "CPC prepared payload 3");
    for (i = 0; i < sizeof trees / sizeof *trees; ++i) {
        join_path(path, sizeof path, root, trees[i]);
        if (!hash_tree(&h, path)) ok = 0;
    }
    if (!hash_tree(&h, runtime)) ok = 0;
    for (i = 0; i < sizeof files / sizeof *files; ++i) {
        join_path(path, sizeof path, root, files[i]);
        if (!hash_file(&h, path)) ok = 0;
    }
    /* Only generated import definitions depend on the installed system DLL. */
    for (i = 0; i < sizeof dlls / sizeof *dlls; ++i) {
        sprintf(name, "%s.def", dlls[i]);
        join_path(path, sizeof path, runtime, name);
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) continue;
        sprintf(name, "third-party/win32-sdk/lib/%s.def", dlls[i]);
        join_path(path, sizeof path, root, name);
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) continue;
        if (!GetSystemDirectoryA(system, sizeof system)) { ok = 0; continue; }
        sprintf(name, "%s.dll", dlls[i]);
        join_path(path, sizeof path, system, name);
        if (!hash_file(&h, path)) ok = 0;
    }
    join_path(path, sizeof path, cache, "payload.bin");
    if (!hash_file(&h, path)) ok = 0;
    join_path(path, sizeof path, cache, "portable-payload.exe");
    if (!hash_file(&h, path)) ok = 0;
    hash_end(&h, digest);
    return ok;
}

static int cache_seal(const char *cache, const char *root, const char *runtime) {
    unsigned char digest[32];
    char path[4096];
    if (!cache_digest(cache, root, runtime, digest)) return 1;
    join_path(path, sizeof path, cache, "inputs.sha256");
    write_file(path, digest, sizeof digest);
    return 0;
}

static int cache_matches(const char *cache, const char *root, const char *runtime) {
    unsigned char expected[32], actual[32];
    char path[4096];
    FILE *file;
    int ok;
    join_path(path, sizeof path, cache, "inputs.sha256");
    file = fopen(path, "rb");
    if (!file) return 0;
    ok = fread(expected, 1, sizeof expected, file) == sizeof expected && fgetc(file) == EOF;
    fclose(file);
    return ok && cache_digest(cache, root, runtime, actual) && !memcmp(expected, actual, sizeof actual);
}
