#ifndef ONE_SOURCE
# define ONE_SOURCE 1
#endif

#if ONE_SOURCE
#include "cprimepp.c"
#include "cprimegen.c"
#include "cprimedbg.c"
#include "cprimeasm.c"
#include "cprimeelf.c"
#include "cprimerun.c"
#ifdef CPRIME_TARGET_X86_64
#include "x86_64-gen.c"
#include "x86_64-link.c"
#include "../backend/x64/x86_64-asm.c"
#else
#error unknown target
#endif
#ifdef CPRIME_TARGET_PE
#include "cprimepe.c"
#endif
#ifdef CPRIME_TARGET_MACHO
#include "cprimemacho.c"
#endif
#endif // ONE_SOURCE 

#include "cprime.h"

//******************************************************
// Global Variables

// XXX: get rid of this ASAP (or maybe not)
ST_DATA struct CPRIMEState *cprime_state;
CPRIME_SEM(static cprime_compile_sem);
// An Array Of Pointers To Memory To Be Free'D After Errors
ST_DATA void **stk_data;
ST_DATA int nb_stk_data;
// Option -D<Num> (For General Development Purposes)
ST_DATA int g_debug;

//******************************************************
#ifdef _WIN32
ST_FUNC char *normalize_slashes(char *path)
{
  char *p;
  for (p = path; *p; ++p)
    if (*p == '\\')
      *p = '/';
  return path;
}

#if defined LIBCPRIME_AS_DLL && !defined CONFIG_CPRIMEDIR
static HMODULE cprime_module;
BOOL WINAPI DllMain (HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
  if (DLL_PROCESS_ATTACH == dwReason)
    cprime_module = hDll;
  return TRUE;
}
#else
#define cprime_module NULL // NULL means executable itself 
#endif

#ifndef CONFIG_CPRIMEDIR
// On Win32, We Suppose The Lib And Includes Are At The Location Of 'Cpc.Exe'
static inline char *config_cprimedir_w32(char *path)
{
  char *p;
  GetModuleFileNameA(cprime_module, path, MAX_PATH);
  p = cprime_basename(normalize_slashes(strlwr(path)));
  if (p > path)
    --p;
  *p = 0;
  return path;
}
#define CONFIG_CPRIMEDIR config_cprimedir_w32(alloca(MAX_PATH))
#endif

#define CPRIME_PORTABLE_VERSION "1.0"
#define CPRIME_PAYLOAD_MAGIC "CPCPAY11"
#define CPRIME_PAYLOAD_HEADER "CPRIMEPKG10"

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t)-1)
#endif

static int cprime_path_exists(const char *path)
{
  DWORD attrs = GetFileAttributesA(path);
  return attrs != INVALID_FILE_ATTRIBUTES;
}

static int cprime_mkdirs(const char *path)
{
  char tmp[MAX_PATH * 4];
  char *p;
  size_t n = strlen(path);
  if (n >= sizeof(tmp))
    return -1;
  strncpy(tmp, path, sizeof(tmp) - 1);
  tmp[sizeof(tmp) - 1] = '\0';
  normalize_slashes(tmp);
  for (p = tmp + 3; *p; ++p)
  {
    if (*p == '/')
    {
      *p = '\0';
      if (*tmp && !CreateDirectoryA(tmp, NULL))
      {
        DWORD e = GetLastError();
        if (e != ERROR_ALREADY_EXISTS)
          return -1;
      }
      *p = '/';
    }
  }
  if (*tmp && !CreateDirectoryA(tmp, NULL))
  {
    DWORD e = GetLastError();
    if (e != ERROR_ALREADY_EXISTS)
      return -1;
  }
  return 0;
}

static int cprime_write_manifest(const char *root)
{
  FILE *fp;
  char path[MAX_PATH * 4];
  snprintf(path, sizeof(path), "%s/manifest.ok", root);
  fp = fopen(path, "wb");
  if (!fp)
    return -1;
  fwrite("ok\n", 1, 3, fp);
  fclose(fp);
  return 0;
}

typedef BOOL(WINAPI *cprime_CreateDecompressorFn)(DWORD, void *, void **);
typedef BOOL(WINAPI *cprime_DecompressFn)(void *, const void *, size_t, void *, size_t, size_t *);
typedef BOOL(WINAPI *cprime_CloseDecompressorFn)(void *);

#define CPRIME_COMPRESSION_FORMAT_LZMS 5

static int cprime_payload_read_u16(const unsigned char *payload, size_t payload_len,
                                size_t *pos, unsigned short *out)
{
  if (*pos > payload_len || payload_len - *pos < 2)
    return -1;
  *out = (unsigned short)(payload[*pos] | (payload[*pos + 1] << 8));
  *pos += 2;
  return 0;
}

static int cprime_payload_read_u32(const unsigned char *payload, size_t payload_len,
                                size_t *pos, unsigned int *out)
{
  if (*pos > payload_len || payload_len - *pos < 4)
    return -1;
  *out = (unsigned int)payload[*pos] |
         ((unsigned int)payload[*pos + 1] << 8) |
         ((unsigned int)payload[*pos + 2] << 16) |
         ((unsigned int)payload[*pos + 3] << 24);
  *pos += 4;
  return 0;
}

static int cprime_payload_read_u64(const unsigned char *payload, size_t payload_len,
                                size_t *pos, unsigned long long *out)
{
  unsigned int lo, hi;
  if (cprime_payload_read_u32(payload, payload_len, pos, &lo) < 0)
    return -1;
  if (cprime_payload_read_u32(payload, payload_len, pos, &hi) < 0)
    return -1;
  *out = ((unsigned long long)hi << 32) | lo;
  return 0;
}

static int cprime_read_payload_memory(const unsigned char *payload, size_t payload_len,
                                   const char *root)
{
  unsigned int file_count, i;
  size_t pos = 0;
  if (cprime_payload_read_u32(payload, payload_len, &pos, &file_count) < 0)
    return -1;
  for (i = 0; i < file_count; ++i)
  {
    unsigned short path_len;
    unsigned long long data_len;
    char rel[1024];
    char out[MAX_PATH * 4];
    char dir[MAX_PATH * 4];
    FILE *fo;
    char *slash;
    if (cprime_payload_read_u16(payload, payload_len, &pos, &path_len) < 0)
      return -1;
    if (cprime_payload_read_u64(payload, payload_len, &pos, &data_len) < 0)
      return -1;
    if (path_len == 0 || path_len >= sizeof(rel))
      return -1;
    if (pos > payload_len || payload_len - pos < path_len)
      return -1;
    memcpy(rel, payload + pos, path_len);
    pos += path_len;
    rel[path_len] = '\0';
    if (data_len > (unsigned long long)(payload_len - pos))
      return -1;
    snprintf(out, sizeof(out), "%s/%s", root, rel);
    normalize_slashes(out);
    strncpy(dir, out, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    slash = cprime_basename(dir);
    if (slash > dir)
    {
      --slash;
      *slash = '\0';
      if (cprime_mkdirs(dir) < 0)
        return -1;
    }
    fo = fopen(out, "wb");
    if (!fo)
      return -1;
    if ((size_t)data_len != fwrite(payload + pos, 1, (size_t)data_len, fo))
    {
      fclose(fo);
      return -1;
    }
    pos += (size_t)data_len;
    fclose(fo);
  }
  return pos == payload_len ? 0 : -1;
}

static unsigned char *cprime_decompress_payload(const unsigned char *compressed,
                                             size_t compressed_len,
                                             size_t uncompressed_len)
{
  HMODULE dll;
  cprime_CreateDecompressorFn pCreateDecompressor;
  cprime_DecompressFn pDecompress;
  cprime_CloseDecompressorFn pCloseDecompressor;
  void *decompressor = NULL;
  unsigned char *payload = NULL;
  size_t actual_len = 0;

  dll = LoadLibraryA("cabinet.dll");
  if (!dll)
    return NULL;
  pCreateDecompressor = (cprime_CreateDecompressorFn)GetProcAddress(dll, "CreateDecompressor");
  pDecompress = (cprime_DecompressFn)GetProcAddress(dll, "Decompress");
  pCloseDecompressor = (cprime_CloseDecompressorFn)GetProcAddress(dll, "CloseDecompressor");
  if (!pCreateDecompressor || !pDecompress || !pCloseDecompressor)
    goto fail;
  if (!pCreateDecompressor(CPRIME_COMPRESSION_FORMAT_LZMS, NULL, &decompressor))
    goto fail;
  payload = cprime_malloc((unsigned long)uncompressed_len);
  if (!payload)
    goto fail;
  if (!pDecompress(decompressor, compressed, compressed_len, payload,
                   uncompressed_len, &actual_len))
    goto fail;
  if (actual_len != uncompressed_len)
    goto fail;
  pCloseDecompressor(decompressor);
  FreeLibrary(dll);
  return payload;

fail:
  if (decompressor)
    pCloseDecompressor(decompressor);
  if (payload)
    cprime_free(payload);
  FreeLibrary(dll);
  return NULL;
}

static char *cprime_portable_root_w32(char *path, size_t n)
{
  DWORD len;
  len = GetEnvironmentVariableA("LOCALAPPDATA", path, (DWORD)n);
  if (!len || len >= n)
    len = GetEnvironmentVariableA("APPDATA", path, (DWORD)n);
  if (!len || len >= n)
  {
    if (!GetTempPathA((DWORD)n, path))
      return NULL;
    len = (DWORD)strlen(path);
  }
  if (len && (path[len - 1] == '\\' || path[len - 1] == '/'))
    path[len - 1] = '\0';
  snprintf(path + strlen(path), n - strlen(path), "\\cpc\\" CPRIME_PORTABLE_VERSION);
  normalize_slashes(path);
  return path;
}

static char *cprime_try_portable_extract_w32(char *out, size_t n)
{
  char exe[MAX_PATH * 4], marker_path[MAX_PATH * 4];
  FILE *fp;
  unsigned long long payload_off;
  char footer_magic[8];
  long long endpos;
  if (!cprime_portable_root_w32(out, n))
    return NULL;
  snprintf(marker_path, sizeof(marker_path), "%s/manifest.ok", out);
  if (cprime_path_exists(marker_path))
    return out;
  if (cprime_mkdirs(out) < 0)
    return NULL;
  GetModuleFileNameA(cprime_module, exe, sizeof(exe));
  fp = fopen(exe, "rb");
  if (!fp)
    return NULL;
  if (fseek(fp, 0, SEEK_END) != 0)
  {
    fclose(fp);
    return NULL;
  }
  endpos = ftell(fp);
  if (endpos < 16)
  {
    fclose(fp);
    return NULL;
  }
  if (fseek(fp, (long)(endpos - 16), SEEK_SET) != 0)
  {
    fclose(fp);
    return NULL;
  }
  if (8 != fread(footer_magic, 1, 8, fp))
  {
    fclose(fp);
    return NULL;
  }
  if (memcmp(footer_magic, CPRIME_PAYLOAD_MAGIC, 8))
  {
    fclose(fp);
    return NULL;
  }
  if (1 != fread(&payload_off, sizeof(payload_off), 1, fp))
  {
    fclose(fp);
    return NULL;
  }
  if (payload_off >= (unsigned long long)endpos)
  {
    fclose(fp);
    return NULL;
  }
  if (fseek(fp, (long)payload_off, SEEK_SET) != 0)
  {
    fclose(fp);
    return NULL;
  }
  {
    unsigned long long unpacked_len64, packed_len64;
    size_t packed_len, unpacked_len;
    unsigned char *packed, *payload;
    int rc;

    if (1 != fread(&unpacked_len64, sizeof(unpacked_len64), 1, fp) ||
        1 != fread(&packed_len64, sizeof(packed_len64), 1, fp))
    {
      fclose(fp);
      return NULL;
    }
    if (unpacked_len64 > (unsigned long long)SIZE_MAX ||
        packed_len64 > (unsigned long long)SIZE_MAX ||
        packed_len64 > (unsigned long long)(endpos - 16 - payload_off - 16))
    {
      fclose(fp);
      return NULL;
    }
    unpacked_len = (size_t)unpacked_len64;
    packed_len = (size_t)packed_len64;
    packed = cprime_malloc((unsigned long)packed_len);
    if (!packed)
    {
      fclose(fp);
      return NULL;
    }
    if (packed_len != fread(packed, 1, packed_len, fp))
    {
      cprime_free(packed);
      fclose(fp);
      return NULL;
    }
    payload = cprime_decompress_payload(packed, packed_len, unpacked_len);
    cprime_free(packed);
    if (!payload)
    {
      fclose(fp);
      return NULL;
    }
    rc = cprime_read_payload_memory(payload, unpacked_len, out);
    cprime_free(payload);
    if (rc < 0)
    {
      fclose(fp);
      return NULL;
    }
  }
  fclose(fp);
  if (cprime_write_manifest(out) < 0)
    return NULL;
  return out;
}

#ifdef CPRIME_IS_NATIVE
static void cprime_add_systemdir(CPRIMEState *s)
{
  char buf[1000];
  GetSystemDirectoryA(buf, sizeof buf);
  cprime_add_library_path(s, normalize_slashes(buf));
}
#endif
#endif

//******************************************************

PUB_FUNC void cprime_enter_state(CPRIMEState *s1)
{
  if (s1->error_set_jmp_enabled)
    return;
  WAIT_SEM(&cprime_compile_sem);
  cprime_state = s1;
}

PUB_FUNC void cprime_exit_state(CPRIMEState *s1)
{
  if (s1->error_set_jmp_enabled)
    return;
  cprime_state = NULL;
  POST_SEM(&cprime_compile_sem);
}

//******************************************************
// Copy A String And Truncate It.
ST_FUNC char *pstrcpy(char *buf, size_t buf_size, const char *s)
{
  char *q, *q_end;
  int c;

  if (buf_size > 0)
  {
    q = buf;
    q_end = buf + buf_size - 1;
    while (q < q_end)
    {
      c = *s++;
      if (c == '\0')
        break;
      *q++ = c;
    }
    *q = '\0';
  }
  return buf;
}

// Strcat And Truncate.
ST_FUNC char *pstrcat(char *buf, size_t buf_size, const char *s)
{
  size_t len;
  len = strlen(buf);
  if (len < buf_size)
    pstrcpy(buf + len, buf_size - len, s);
  return buf;
}

ST_FUNC char *pstrncpy(char *out, size_t buf_size, const char *s, size_t num)
{
  if (num >= buf_size)
    num = buf_size - 1;
  memcpy(out, s, num);
  out[num] = '\0';
  return out;
}

// Extract The Basename Of A File
PUB_FUNC char *cprime_basename(const char *name)
{
  char *p = strchr(name, 0);
  while (p > name && !IS_DIRSEP(p[-1]))
    --p;
  return p;
}

/* extract extension part of a file
 *
 * (if no extension, return pointer to end-of-string)
 */
PUB_FUNC char *cprime_fileextension (const char *name)
{
  char *b = cprime_basename(name);
  char *e = strrchr(b, '.');
  return e ? e : strchr(b, 0);
}

ST_FUNC char *cprime_load_text(int fd)
{
  int len = lseek(fd, 0, SEEK_END);
  char *buf = load_data(fd, 0, len + 1);
  buf[len] = 0;
  return buf;
}

// replace *pp by copy of 'str' or NULL
static void cprime_set_str(char **pp, const char *str)
{
  cprime_free(*pp);
  *pp = str ? cprime_strdup(str) : NULL;
}

// Set/Append 'Str' To *Pp (Separated By 'Sep' Unless 0)
static void cprime_concat_str(char **pp, const char *str, int sep)
{
  int l = *pp ? strlen(*pp) + !!sep : 0;
  *pp = cprime_realloc(*pp, l + strlen(str) + 1);
  if (l && sep) ((*pp)[l - 1] = sep);
  strcpy(*pp + l, str);
}

//******************************************************
// Memory Management

// We'Ll Need The Actual Versions For A Minute
#undef free
#undef realloc

static void *default_reallocator(void *ptr, unsigned long size)
{
  void *ptr1;
  if (size == 0)
  {
    free(ptr);
    ptr1 = NULL;
  }
  else
  {
    ptr1 = realloc(ptr, size);
    if (!ptr1)
    {
      fprintf(stderr, "memory full\n");
      exit (1);
    }
  }
  return ptr1;
}

ST_FUNC void libc_free(void *ptr)
{
  free(ptr);
}

// Defined To Be Not Used
#define free(p) use_cprime_free(p)
#define realloc(p, s) use_cprime_realloc(p, s)

// global so that every cprime_alloc()/cprime_free() call doesn't need to be changed
static void *(*reallocator)(void *, unsigned long) = default_reallocator;

LIBCPRIMEAPI void cprime_set_realloc(CPRIMEReallocFunc *my_realloc)
{
  reallocator = my_realloc ? my_realloc : default_reallocator;
}

// in case MEM_DEBUG is #defined
#undef cprime_free
#undef cprime_malloc
#undef cprime_realloc
#undef cprime_mallocz
#undef cprime_strdup

PUB_FUNC void cprime_free(void *ptr)
{
  reallocator(ptr, 0);
}

PUB_FUNC void *cprime_malloc(unsigned long size)
{
  return reallocator(0, size);
}

PUB_FUNC void *cprime_realloc(void *ptr, unsigned long size)
{
  return reallocator(ptr, size);
}

PUB_FUNC void *cprime_mallocz(unsigned long size)
{
  void *ptr;
  ptr = cprime_malloc(size);
  if (size)
    memset(ptr, 0, size);
  return ptr;
}

PUB_FUNC char *cprime_strdup(const char *str)
{
  char *ptr;
  ptr = cprime_malloc(strlen(str) + 1);
  strcpy(ptr, str);
  return ptr;
}

#ifdef MEM_DEBUG

#define MEM_DEBUG_MAGIC1 0xFEEDDEB1
#define MEM_DEBUG_MAGIC2 0xFEEDDEB2
#define MEM_DEBUG_MAGIC3 0xFEEDDEB3
#define MEM_DEBUG_FILE_LEN 40
#define MEM_DEBUG_CHECK3(header) \
    (((unsigned char *) header->magic3) + header->size)
#define MEM_USER_PTR(header) \
    ((char *)header + offsetof(mem_debug_header_t, magic3))
#define MEM_HEADER_PTR(ptr) \
    (mem_debug_header_t *)((char*)ptr - offsetof(mem_debug_header_t, magic3))

struct mem_debug_header
{
  unsigned magic1;
  unsigned size;
  struct mem_debug_header *prev;
  struct mem_debug_header *next;
  int line_num;
  char file_name[MEM_DEBUG_FILE_LEN];
  unsigned magic2;
  ALIGNED(16) unsigned char magic3[4];
};

typedef struct mem_debug_header mem_debug_header_t;

CPRIME_SEM(static mem_sem);
static mem_debug_header_t *mem_debug_chain;
static unsigned mem_cur_size;
static unsigned mem_max_size;
static int nb_states;

static mem_debug_header_t *malloc_check(void *ptr, const char *msg)
{
  mem_debug_header_t * header = MEM_HEADER_PTR(ptr);
  if (header->magic1 != MEM_DEBUG_MAGIC1 ||
      header->magic2 != MEM_DEBUG_MAGIC2 ||
      read32le(MEM_DEBUG_CHECK3(header)) != MEM_DEBUG_MAGIC3 ||
      header->size == (unsigned) - 1)
  {
    fprintf(stderr, "%s check failed\n", msg);
    if (header->magic1 == MEM_DEBUG_MAGIC1)
      fprintf(stderr, "%s:%u: block allocated here.\n",
              header->file_name, header->line_num);
    exit(1);
  }
  return header;
}

PUB_FUNC void *cprime_malloc_debug(unsigned long size, const char *file, int line)
{
  int ofs;
  mem_debug_header_t *header;
  if (!size)
    return NULL;
  header = cprime_malloc(sizeof(mem_debug_header_t) + size);
  header->magic1 = MEM_DEBUG_MAGIC1;
  header->magic2 = MEM_DEBUG_MAGIC2;
  header->size = size;
  write32le(MEM_DEBUG_CHECK3(header), MEM_DEBUG_MAGIC3);
  header->line_num = line;
  ofs = strlen(file) + 1 - MEM_DEBUG_FILE_LEN;
  strcpy(header->file_name, file + (ofs > 0 ? ofs : 0));
  WAIT_SEM(&mem_sem);
  header->next = mem_debug_chain;
  header->prev = NULL;
  if (header->next)
    header->next->prev = header;
  mem_debug_chain = header;
  mem_cur_size += size;
  if (mem_cur_size > mem_max_size)
    mem_max_size = mem_cur_size;
  POST_SEM(&mem_sem);
  return MEM_USER_PTR(header);
}

PUB_FUNC void cprime_free_debug(void *ptr)
{
  mem_debug_header_t *header;
  if (!ptr)
    return;
  header = malloc_check(ptr, "cprime_free");
  WAIT_SEM(&mem_sem);
  mem_cur_size -= header->size;
  header->size = (unsigned) - 1;
  if (header->next)
    header->next->prev = header->prev;
  if (header->prev)
    header->prev->next = header->next;
  if (header == mem_debug_chain)
    mem_debug_chain = header->next;
  POST_SEM(&mem_sem);
  cprime_free(header);
}

PUB_FUNC void *cprime_mallocz_debug(unsigned long size, const char *file, int line)
{
  void *ptr;
  ptr = cprime_malloc_debug(size, file, line);
  if (size)
    memset(ptr, 0, size);
  return ptr;
}

PUB_FUNC void *cprime_realloc_debug(void *ptr, unsigned long size, const char *file, int line)
{
  mem_debug_header_t *header;
  int mem_debug_chain_update = 0;

  if (!ptr)
    return cprime_malloc_debug(size, file, line);
  if (!size)
  {
    cprime_free_debug(ptr);
    return NULL;
  }
  header = malloc_check(ptr, "cprime_realloc");
  WAIT_SEM(&mem_sem);
  mem_cur_size -= header->size;
  mem_debug_chain_update = (header == mem_debug_chain);
  header = cprime_realloc(header, sizeof(mem_debug_header_t) + size);
  header->size = size;
  write32le(MEM_DEBUG_CHECK3(header), MEM_DEBUG_MAGIC3);
  if (header->next)
    header->next->prev = header;
  if (header->prev)
    header->prev->next = header;
  if (mem_debug_chain_update)
    mem_debug_chain = header;
  mem_cur_size += size;
  if (mem_cur_size > mem_max_size)
    mem_max_size = mem_cur_size;
  POST_SEM(&mem_sem);
  return MEM_USER_PTR(header);
}

PUB_FUNC char *cprime_strdup_debug(const char *str, const char *file, int line)
{
  char *ptr;
  ptr = cprime_malloc_debug(strlen(str) + 1, file, line);
  strcpy(ptr, str);
  return ptr;
}

PUB_FUNC void cprime_memcheck(int d)
{
  WAIT_SEM(&mem_sem);
  nb_states += d;
  if (0 == nb_states && mem_cur_size)
  {
    mem_debug_header_t *header = mem_debug_chain;
    fflush(stdout);
    fprintf(stderr, "MEM_DEBUG: mem_leak= %d bytes, mem_max_size= %d bytes\n",
            mem_cur_size, mem_max_size);
    while (header)
    {
      fprintf(stderr, "%s:%u: error: %u bytes leaked\n",
              header->file_name, header->line_num, header->size);
      header = header->next;
    }
    fflush(stderr);
    mem_cur_size = 0;
    mem_max_size = 0;
    mem_debug_chain = NULL;
#if MEM_DEBUG-0 == 2
    exit(2);
#endif
  }
  POST_SEM(&mem_sem);
}

// Restore The Debug Versions
#define cprime_free(ptr)           cprime_free_debug(ptr)
#define cprime_malloc(size)        cprime_malloc_debug(size, __FILE__, __LINE__)
#define cprime_mallocz(size)       cprime_mallocz_debug(size, __FILE__, __LINE__)
#define cprime_realloc(ptr,size)   cprime_realloc_debug(ptr, size, __FILE__, __LINE__)
#define cprime_strdup(str)         cprime_strdup_debug(str, __FILE__, __LINE__)

#endif // MEM_DEBUG 

#ifdef _WIN32
# define realpath(file, buf) _fullpath(buf, file, 260)
#endif

// For #Pragma Once
ST_FUNC int normalized_PATHCMP(const char *f1, const char *f2)
{
  char *p1, *p2;
  int ret = 1;
  if (!!(p1 = realpath(f1, NULL)))
  {
    if (!!(p2 = realpath(f2, NULL)))
    {
      ret = PATHCMP(p1, p2);
      libc_free(p2); // Realpath() Requirement
    }
    libc_free(p1);
  }
  return ret;
}

//******************************************************
// Dynarrays

ST_FUNC void dynarray_add(void *ptab, int *nb_ptr, void *data)
{
  int nb, nb_alloc;
  void **pp;

  nb = *nb_ptr;
  pp = *(void ***)ptab;
  // every power of two we double array size
  if ((nb & (nb - 1)) == 0)
  {
    if (!nb)
      nb_alloc = 1;
    else
      nb_alloc = nb * 2;
    pp = cprime_realloc(pp, nb_alloc *sizeof(void *));
    *(void ***)ptab = pp;
  }
  pp[nb++] = data;
  *nb_ptr = nb;
}

ST_FUNC void dynarray_reset(void *pp, int *n)
{
  void **p;
  for (p = *(void * **)pp; *n; ++p, --*n)
    if (*p)
      cprime_free(*p);
  cprime_free(*(void **)pp);
  *(void **)pp = NULL;
}

static void dynarray_split(char ***argv, int *argc, const char *p, int sep)
{
  int qot, c;
  CString str;
  for (;;)
  {
    while (c = (unsigned char) * p, c <= ' ' && c != '\0')
      ++p;
    if (c == '\0')
      break;
    cstr_new(&str);
    qot = 0;
    do
    {
      ++p;
      if (sep)   // e.g. to split -Wl,-opt,arg
      {
        if (c == sep)
          break;
      }
      else
      {
        // E.G. For Tcc_Set_Options() Or "Cpc @Listfile"
        if (c == '\\' && (*p == '"' || *p == '\\'))
          c = *p++;
        else if (c == '"')
        {
          qot ^= 1;
          continue;
        }
        else if (c <= ' ' && !qot)
          break;
      }
      cstr_ccat(&str, c);
    }
    while (c = (unsigned char) * p, c != '\0');
    cstr_ccat(&str, '\0');
    //printf("<%s>\n", str.data);
    dynarray_add(argv, argc, str.data);
  }
}

static void cprime_split_path(CPRIMEState *s, void *p_ary, int *p_nb_ary, const char *in)
{
  const char *p;
  do
  {
    int c;
    CString str;

    cstr_new(&str);
    for (p = in; c = *p, c != '\0' && c != PATHSEP[0]; ++p)
    {
      if (c == '{' && p[1] && p[2] == '}')
      {
        c = p[1], p += 2;
        if (c == 'B')
          cstr_cat(&str, s->cprime_lib_path, -1);
        if (c == 'R')
          cstr_cat(&str, CONFIG_SYSROOT, -1);
        if (c == 'f' && file)
        {
          // Substitute Current File'S Dir
          const char *f = file->true_filename;
          const char *b = cprime_basename(f);
          if (b > f)
            cstr_cat(&str, f, b - f - 1);
          else
            cstr_cat(&str, ".", 1);
        }
      }
      else
        cstr_ccat(&str, c);
    }
    if (str.size)
    {
      cstr_ccat(&str, '\0');
      dynarray_add(p_ary, p_nb_ary, str.data);
    }
    in = p + 1;
  }
  while (*p);
}

//******************************************************
// Warning / Error

// Warn_... Option Bits
#define WARN_ON  1 // warning is on (-Woption) 
#define WARN_ERR 2 // warning is an error (-Werror=option) 
#define WARN_NOE 4 // warning is not an error (-Wno-error=option) 

// Error1() Modes
enum { ERROR_WARN, ERROR_NOABORT, ERROR_ERROR };

static void error1(int mode, const char *fmt, va_list ap)
{
  BufferedFile **pf, *f;
  CPRIMEState *s1 = cprime_state;
  CString cs;
  int line = 0;

  cprime_exit_state(s1);

  if (mode == ERROR_WARN)
  {
    if (s1->warn_error)
      mode = ERROR_ERROR;
    if (s1->warn_num)
    {
      // Handle Tcc_Warning_C(Warn_Option)(Fmt, ...)
      int wopt = *(&s1->warn_none + s1->warn_num);
      s1->warn_num = 0;
      if (0 == (wopt & WARN_ON))
        return;
      if (wopt & WARN_ERR)
        mode = ERROR_ERROR;
      if (wopt & WARN_NOE)
        mode = ERROR_WARN;
    }
    if (s1->warn_none)
      return;
  }

  cstr_new(&cs);
  if (fmt[0] == '%' && fmt[1] == 'i' && fmt[2] == ':')
    line = va_arg(ap, int), fmt += 3;
  f = NULL;
  if (s1->error_set_jmp_enabled)   // we're called while parsing a file
  {
    // use upper file if inline ":asm:" or token ":paste:"
    for (f = file; f && f->filename[0] == ':'; f = f->prev)
      ;
  }
  if (f)
  {
    for (pf = s1->include_stack; pf < s1->include_stack_ptr; pf++)
      cstr_printf(&cs, "In file included from %s:%d:\n",
                  (*pf)->filename, (*pf)->line_num - 1);
    if (0 == line)
      line = f->line_num - ((tok_flags & TOK_FLAG_BOL) && !macro_ptr);
    cstr_printf(&cs, "%s:%d: ", f->filename, line);
  }
  else if (s1->current_filename)
    cstr_printf(&cs, "%s: ", s1->current_filename);
  else
    cstr_printf(&cs, "cpc: ");
  cstr_printf(&cs, mode == ERROR_WARN ? "warning: " : "error: ");
  if (pp_expr > 1)
    pp_error(&cs); // Special Handler For Preprocessor Expression Errors
  else
    cstr_vprintf(&cs, fmt, ap);
  if (!s1->error_func)
  {
    // Default Case: Stderr
    if (s1 && s1->output_type == CPRIME_OUTPUT_PREPROCESS && s1->ppfp == stdout)
      printf("\n"); // print a newline during cpc -E
    fflush(stdout); // Flush -V Output
    fprintf(stderr, "%s\n", (char *)cs.data);
    fflush(stderr); // Print Error/Warning Now (Win32)
  }
  else
    s1->error_func(s1->error_opaque, (char *)cs.data);
  cstr_free(&cs);
  if (mode != ERROR_WARN)
    s1->nb_errors++;
  if (mode == ERROR_ERROR && s1->error_set_jmp_enabled)
  {
    while (nb_stk_data)
      cprime_free(*(void * *)stk_data[--nb_stk_data]);
    longjmp(s1->error_jmp_buf, 1);
  }
}

LIBCPRIMEAPI void cprime_set_error_func(CPRIMEState *s, void *error_opaque, CPRIMEErrorFunc *error_func)
{
  s->error_opaque = error_opaque;
  s->error_func = error_func;
}

// Error Without Aborting Current Compilation
PUB_FUNC int _cprime_error_noabort(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  error1(ERROR_NOABORT, fmt, ap);
  va_end(ap);
  return -1;
}

#undef _cprime_error
PUB_FUNC void _cprime_error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  error1(ERROR_ERROR, fmt, ap);
  exit(1);
}
#define _cprime_error use_cprime_error_noabort

PUB_FUNC void _cprime_warning(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  error1(ERROR_WARN, fmt, ap);
  va_end(ap);
}


//******************************************************
// I/O layer

ST_FUNC void cprime_open_bf(CPRIMEState *s1, const char *filename, int initlen)
{
  BufferedFile *bf;
  int buflen = initlen ? initlen : IO_BUF_SIZE;

  bf = cprime_mallocz(sizeof(BufferedFile) + buflen);
  bf->buf_ptr = bf->buffer;
  bf->buf_end = bf->buffer + initlen;
  bf->buf_end[0] = CH_EOB; // Put Eob Symbol
  pstrcpy(bf->filename, sizeof(bf->filename), filename);
#ifdef _WIN32
  normalize_slashes(bf->filename);
#endif
  bf->true_filename = bf->filename;
  bf->line_num = 1;
  bf->ifdef_stack_ptr = s1->ifdef_stack_ptr;
  bf->fd = -1;
  bf->prev = file;
  bf->prev_tok_flags = tok_flags;
  file = bf;
  tok_flags = TOK_FLAG_BOL | TOK_FLAG_BOF;
}

ST_FUNC void cprime_close(void)
{
  CPRIMEState *s1 = cprime_state;
  BufferedFile *bf = file;
  if (bf->fd > 0)
  {
    close(bf->fd);
    total_lines += bf->line_num - 1;
  }
  if (bf->true_filename != bf->filename)
    cprime_free(bf->true_filename);
  file = bf->prev;
  tok_flags = bf->prev_tok_flags;
  cprime_free(bf);
}

static int _cprime_open(CPRIMEState *s1, const char *filename)
{
  int fd;
  if (strcmp(filename, "-") == 0)
    fd = 0, filename = "<stdin>";
  else
    fd = open(filename, O_RDONLY | O_BINARY);
  if ((s1->verbose == 2 && fd >= 0) || s1->verbose == 3)
    printf("%s %*s%s\n", fd < 0 ? "nf" : "->",
           (int)(s1->include_stack_ptr - s1->include_stack), "", filename);
  return fd;
}

ST_FUNC int cprime_open(CPRIMEState *s1, const char *filename)
{
  int fd = _cprime_open(s1, filename);
  if (fd < 0)
    return -1;
  cprime_open_bf(s1, filename, 0);
  file->fd = fd;
  return 0;
}

// compile the file opened in 'file'. Return non zero if errors.
static int cprime_compile(CPRIMEState *s1, int filetype, const char *str, int fd, const char *filename)
{
  /* Here we enter the code section where we use the global variables for
     parsing and code generation (cprimepp.c, cprimegen.c, <target>-gen.c).
     Other threads need to wait until we're done.

     Alternatively we could use thread local storage for those global
     variables, which may or may not have advantages */

  cprime_enter_state(s1);
  s1->error_set_jmp_enabled = 1;

  if (setjmp(s1->error_jmp_buf) == 0)
  {

    if (fd == -1)
    {
      int len = strlen(str);
      cprime_open_bf(s1, filename ? filename : "<string>", len);
      memcpy(file->buffer, str, len);
      if (s1->do_debug && filename)
      {
        FILE *fp = fopen(filename, "w");

        if (fp)
        {
          fputs(str, fp);
          fclose(fp);
        }
      }
    }
    else
    {
      cprime_open_bf(s1, str, 0);
      file->fd = fd;
    }

    preprocess_start(s1, filetype);
    cprimegen_init(s1);

    if (s1->output_type == CPRIME_OUTPUT_PREPROCESS)
      cprime_preprocess(s1);
    else
    {
      cprimeelf_begin_file(s1);
      if (filetype & (AFF_TYPE_ASM | AFF_TYPE_ASMPP))
        cprime_assemble(s1, !!(filetype & AFF_TYPE_ASMPP));
      else
        cprimegen_compile(s1);
      cprimeelf_end_file(s1);
    }
  }
  cprimegen_finish(s1);
  preprocess_end(s1);
  s1->error_set_jmp_enabled = 0;
  cprime_exit_state(s1);
  return s1->nb_errors != 0 ? -1 : 0;
}

LIBCPRIMEAPI int cprime_compile_string(CPRIMEState *s, const char *str)
{
  return cprime_compile(s, s->filetype, str, -1, NULL);
}

LIBCPRIMEAPI int cprime_compile_string_file(CPRIMEState *s, const char *str, const char *filename)
{
  return cprime_compile(s, s->filetype, str, -1, filename);
}

// define a preprocessor symbol. value can be NULL, sym can be "sym=val"
LIBCPRIMEAPI void cprime_define_symbol(CPRIMEState *s1, const char *sym, const char *value)
{
  const char *eq;
  if (NULL == (eq = strchr(sym, '=')))
    eq = strchr(sym, 0);
  if (NULL == value)
    value = *eq ? eq + 1 : "1";
  cstr_printf(&s1->cmdline_defs, "#define %.*s %s\n", (int)(eq - sym), sym, value);
}

// Undefine A Preprocessor Symbol
LIBCPRIMEAPI void cprime_undefine_symbol(CPRIMEState *s1, const char *sym)
{
  cstr_printf(&s1->cmdline_defs, "#undef %s\n", sym);
}


LIBCPRIMEAPI CPRIMEState *cprime_new(void)
{
  CPRIMEState *s;

  s = cprime_mallocz(sizeof(CPRIMEState));
#ifdef MEM_DEBUG
  cprime_memcheck(1);
#endif

#undef gnu_ext
  s->gnu_ext = 1;
  s->cprime_ext = 1;
  s->nocommon = 1;
  s->dollars_in_identifiers = 1; // On By Default Like In Gcc/Clang
  s->cversion = 199901; // Default Unless -Std=C11 Is Supplied
  s->warn_implicit_function_declaration = 1;
  s->warn_discarded_qualifiers = 1;
  s->ms_extensions = 1;
  s->unwind_tables = 1;

#ifdef CHAR_IS_UNSIGNED
  s->char_is_unsigned = 1;
#endif
  // enable this if you want symbols with leading underscore on windows:
#if defined CPRIME_TARGET_MACHO // || defined CPRIME_TARGET_PE 
  s->leading_underscore = 1;
#endif
#ifdef CONFIG_NEW_DTAGS
  s->enable_new_dtags = 1;
#endif
  s->ppfp = stdout;
  // Might Be Used In Error() Before Preprocess_Start()
  s->include_stack_ptr = s->include_stack;

  cprime_set_lib_path(s, CONFIG_CPRIMEDIR);
#ifdef _WIN32
  {
    char portable_root[MAX_PATH * 4];
    char *pr = cprime_try_portable_extract_w32(portable_root, sizeof(portable_root));
    if (pr)
      cprime_set_lib_path(s, pr);
  }
#endif
#ifdef CONFIG_CPRIME_SWITCHES // Predefined Options 
  cprime_set_options(s, CONFIG_CPRIME_SWITCHES);
#endif
  return s;
}

LIBCPRIMEAPI void cprime_delete(CPRIMEState *s1)
{
  // Free Sections
  cprimeelf_delete(s1);

  // Free Library Paths
  dynarray_reset(&s1->library_paths, &s1->nb_library_paths);
  dynarray_reset(&s1->crt_paths, &s1->nb_crt_paths);

  // Free Include Paths
  dynarray_reset(&s1->include_paths, &s1->nb_include_paths);
  dynarray_reset(&s1->sysinclude_paths, &s1->nb_sysinclude_paths);

  cprime_free(s1->cprime_lib_path);
  cprime_free(s1->soname);
  cprime_free(s1->rpath);
  cprime_free(s1->elfint);
  cprime_free(s1->elf_entryname);
  cprime_free(s1->init_symbol);
  cprime_free(s1->fini_symbol);
  cprime_free(s1->mapfile);
  cprime_free(s1->outfile);
  cprime_free(s1->deps_outfile);
#if defined CPRIME_TARGET_MACHO
  cprime_free(s1->install_name);
#endif
  dynarray_reset(&s1->files, &s1->nb_files);
  dynarray_reset(&s1->target_deps, &s1->nb_target_deps);
  dynarray_reset(&s1->pragma_libs, &s1->nb_pragma_libs);
  dynarray_reset(&s1->argv, &s1->argc);
  dynarray_reset(&s1->link_argv, &s1->link_argc);
  cstr_free(&s1->cmdline_defs);
  cstr_free(&s1->cmdline_incl);
  cprime_free(s1->dState);
#ifdef CPRIME_IS_NATIVE
  // Free Runtime Memory
  cprime_run_free(s1);
#endif
  // Free Loaded Dlls Array
  dynarray_reset(&s1->loaded_dlls, &s1->nb_loaded_dlls);
  cprime_free(s1);
#ifdef MEM_DEBUG
  cprime_memcheck(-1);
#endif
}

LIBCPRIMEAPI int cprime_set_output_type(CPRIMEState *s, int output_type)
{
#ifdef CONFIG_CPRIME_PIE
  if (output_type == CPRIME_OUTPUT_EXE)
    output_type |= CPRIME_OUTPUT_DYN;
#endif
  s->output_type = output_type;

  if (!s->nostdinc)
  {
    // Default Include Paths
    // -Isystem Paths Have Already Been Handled
    cprime_add_sysinclude_path(s, CONFIG_CPRIME_SYSINCLUDEPATHS);
  }

  if (output_type == CPRIME_OUTPUT_PREPROCESS)
  {
    s->do_debug = 0;
    return 0;
  }

  // Add Sections
  cprimeelf_new(s);

  if (output_type == CPRIME_OUTPUT_OBJ)
  {
    // Always Elf For Objects
    s->output_format = CPRIME_OUTPUT_FORMAT_ELF;
    return 0;
  }

  if (!s->nostdlib_paths)
    cprime_add_library_path(s, CONFIG_CPRIME_LIBPATHS);

#ifdef CPRIME_TARGET_PE
# ifdef CPRIME_IS_NATIVE
  // Allow Linking With System Dll'S Directly
  cprime_add_systemdir(s);
# endif
#elif defined CPRIME_TARGET_MACHO
# ifdef CPRIME_IS_NATIVE
  cprime_add_macos_sdkpath(s);
# endif
#else
  // Paths For Crt Objects
  cprime_split_path(s, &s->crt_paths, &s->nb_crt_paths, CONFIG_CPRIME_CRTPREFIX);
  if (output_type != CPRIME_OUTPUT_MEMORY && !s->nostdlib)
    cprimeelf_add_crtbegin(s);
#endif
  return 0;
}

LIBCPRIMEAPI int cprime_add_include_path(CPRIMEState *s, const char *pathname)
{
  cprime_split_path(s, &s->include_paths, &s->nb_include_paths, pathname);
  return 0;
}

LIBCPRIMEAPI int cprime_add_sysinclude_path(CPRIMEState *s, const char *pathname)
{
  cprime_split_path(s, &s->sysinclude_paths, &s->nb_sysinclude_paths, pathname);
  return 0;
}

LIBCPRIMEAPI int cprime_add_library_path(CPRIMEState *s, const char *pathname)
{
  cprime_split_path(s, &s->library_paths, &s->nb_library_paths, pathname);
  return 0;
}

LIBCPRIMEAPI void cprime_set_lib_path(CPRIMEState *s, const char *path)
{
  cprime_set_str(&s->cprime_lib_path, path);
}

// add/update a 'DLLReference', Just find if level == -1
ST_FUNC DLLReference *cprime_add_dllref(CPRIMEState *s1, const char *dllname, int level)
{
  DLLReference *ref = NULL;
  int i;
  for (i = 0; i < s1->nb_loaded_dlls; i++)
    if (0 == strcmp(s1->loaded_dlls[i]->name, dllname))
    {
      ref = s1->loaded_dlls[i];
      break;
    }
  if (level == -1)
    return ref;
  if (ref)
  {
    if (level < ref->level)
      ref->level = level;
    ref->found = 1;
    return ref;
  }
  ref = cprime_mallocz(sizeof(DLLReference) + strlen(dllname));
  strcpy(ref->name, dllname);
  dynarray_add(&s1->loaded_dlls, &s1->nb_loaded_dlls, ref);
  ref->level = level;
  ref->index = s1->nb_loaded_dlls;
  return ref;
}

static int cprime_add_binary(CPRIMEState *s1, int flags, const char *filename, int fd)
{
  ObjW(Ehdr) ehdr;
  int obj_type;
  const char *saved_filename = s1->current_filename;
  int ret = 0;

  s1->current_filename = filename;
  obj_type = cprime_object_type(fd, &ehdr);
  lseek(fd, 0, SEEK_SET);

  switch (obj_type)
  {

  case AFF_BINTYPE_REL:
    ret = cprime_load_object_file(s1, fd, 0);
    break;

  case AFF_BINTYPE_AR:
    ret = cprime_load_archive(s1, fd, !(flags &AFF_WHOLE_ARCHIVE));
    break;

#if defined CPRIME_TARGET_UNIX
  case AFF_BINTYPE_DYN:
    if (s1->output_type == CPRIME_OUTPUT_MEMORY)
    {
#ifdef CPRIME_IS_NATIVE
      void* dl = dlopen(filename, RTLD_GLOBAL | RTLD_LAZY);
      if (dl)
        cprime_add_dllref(s1, filename, 0)->handle = dl;
      else
        ret = FILE_NOT_RECOGNIZED;
#endif
    }
    else
      ret = cprime_load_dll(s1, fd, filename, (flags & AFF_REFERENCED_DLL) != 0);
    break;

  default:
    // treat it as a linker script if not recognized
    ret = cprime_load_ldscript(s1, fd);
    break;

#elif defined CPRIME_TARGET_MACHO
  case AFF_BINTYPE_DYN:
case_dyn_or_tbd:
    if (s1->output_type == CPRIME_OUTPUT_MEMORY)
    {
#ifdef CPRIME_IS_NATIVE
      void* dl;
      const char* soname = filename;
      char *tmp = 0;
      if (obj_type != AFF_BINTYPE_DYN)
      {
        tmp = macho_tbd_soname(fd);
        if (tmp)
          soname = tmp;
      }
      dl = dlopen(soname, RTLD_GLOBAL | RTLD_LAZY);
      if (dl)
        cprime_add_dllref(s1, soname, 0)->handle = dl;
      else
        ret = FILE_NOT_RECOGNIZED;
      cprime_free(tmp);
#endif
    }
    else if (obj_type == AFF_BINTYPE_DYN)
      ret = macho_load_dll(s1, fd, filename, (flags & AFF_REFERENCED_DLL) != 0);
    else
      ret = macho_load_tbd(s1, fd, filename, (flags & AFF_REFERENCED_DLL) != 0);
    if (ret)
      ret = FILE_NOT_RECOGNIZED;
    break;

  default:
  {
    const char *ext = cprime_fileextension(filename);
    if (!strcmp(ext, ".tbd"))
      goto case_dyn_or_tbd;
    if (!strcmp(ext, ".dylib"))
    {
      obj_type = AFF_BINTYPE_DYN;
      goto case_dyn_or_tbd;
    }
    ret = FILE_NOT_RECOGNIZED;
    break;
  }

#elif defined CPRIME_TARGET_PE
  default:
    if (pe_load_file(s1, fd, filename))
      ret = FILE_NOT_RECOGNIZED;
    break;
#endif

#ifdef CPRIME_TARGET_COFF
  case AFF_BINTYPE_C67:
    ret = cprime_load_coff(s1, fd);
    break;
#endif
  }

  close(fd);
  s1->current_filename = saved_filename;
  if (ret == FILE_NOT_RECOGNIZED)
    return cprime_error_noabort("%s: unrecognized file type", filename);
  return ret;
}

// OpenBSD: choose latest from libxxx.so.x.y versions
#if defined TARGETOS_OpenBSD && !defined _WIN32
#include <glob.h>
static int cprime_glob_so(CPRIMEState *s1, const char *pattern, char *buf, int size)
{
  const char *star;
  glob_t g;
  char *p;
  int i, v, v1, v2, v3;

  star = strchr(pattern, '*');
  if (!star || glob(pattern, 0, NULL, &g))
    return -1;
  for (v = -1, i = 0; i < g.gl_pathc; ++i)
  {
    p = g.gl_pathv[i];
    if (2 != sscanf(p + (star - pattern), "%d.%d.%d", &v1, &v2, &v3))
      continue;
    if ((v1 = v1 * 1000 + v2) > v)
      v = v1, pstrcpy(buf, size, p);
  }
  globfree(&g);
  return v;
}
#endif

static int guess_filetype(const char *filename)
{
  int filetype = 0;
  if (1)
  {
    // Use A File Extension To Detect A Filetype
    const char *ext = cprime_fileextension(filename);
    if (ext[0])
    {
      ext++;
      if (!strcmp(ext, "S"))
        filetype = AFF_TYPE_ASMPP;
      else if (!strcmp(ext, "s"))
        filetype = AFF_TYPE_ASM;
      else if (!PATHCMP(ext, "c")
               || !PATHCMP(ext, "h")
               || !PATHCMP(ext, "i"))
        filetype = AFF_TYPE_C;
      else
        filetype |= AFF_TYPE_BIN;
    }
    else
      filetype = AFF_TYPE_C;
  }
  return filetype;
}

ST_FUNC int cprime_add_file_internal(CPRIMEState *s1, const char *filename, int flags)
{
  int fd;

#if defined TARGETOS_OpenBSD && !defined _WIN32
  char buf[1024];
  if (cprime_glob_so(s1, filename, buf, sizeof buf) >= 0)
    filename = buf;
#endif

  if (0 == (flags & AFF_TYPE_MASK))
    flags |= guess_filetype(filename);

  // ignore binary files with -E
  if (s1->output_type == CPRIME_OUTPUT_PREPROCESS
      && (flags & AFF_TYPE_BIN))
    return 0;

  // Open The File
  fd = _cprime_open(s1, filename);
  if (fd < 0)
  {
    if (flags & AFF_PRINT_ERROR)
      cprime_error_noabort("file '%s' not found", filename);
    return FILE_NOT_FOUND;
  }

  if (flags & AFF_TYPE_BIN)
    return cprime_add_binary(s1, flags, filename, fd);

  dynarray_add(&s1->target_deps, &s1->nb_target_deps, cprime_strdup(filename));
  return cprime_compile(s1, flags, filename, fd, NULL);
}

LIBCPRIMEAPI int cprime_add_file(CPRIMEState *s, const char *filename)
{
  return cprime_add_file_internal(s, filename, s->filetype | AFF_PRINT_ERROR);
}

static int cprime_add_library_internal(CPRIMEState *s1, const char *fmt,
                                    const char *filename, int flags, char **paths, int nb_paths)
{
  char buf[1024];
  int i, ret;

  for (i = 0; i < nb_paths; i++)
  {
    snprintf(buf, sizeof(buf), fmt, paths[i], filename);
    ret = cprime_add_file_internal(s1, buf, flags & ~AFF_PRINT_ERROR);
    if (ret != FILE_NOT_FOUND)
      return ret;
  }
  if (flags & AFF_PRINT_ERROR)
    cprime_error_noabort("%s '%s' not found",
                      flags & AFF_TYPE_LIB ? "library" : "file", filename);
  return FILE_NOT_FOUND;
}

// find and load a dll. Return non zero if not found
ST_FUNC int cprime_add_dll(CPRIMEState *s, const char *filename, int flags)
{
  return cprime_add_library_internal(s, "%s/%s", filename, flags,
                                  s->library_paths, s->nb_library_paths);
}

// Find [Cross-]Libcprime1.A And Cpc Helper Objects In Library Path
ST_FUNC int cprime_add_support(CPRIMEState *s1, const char *filename)
{
  char buf[100];
  if (CONFIG_CPRIME_CROSSPREFIX[0])
    filename = strcat(strcpy(buf, CONFIG_CPRIME_CROSSPREFIX), filename);
  return cprime_add_dll(s1, filename, AFF_PRINT_ERROR);
}

#ifdef CPRIME_TARGET_UNIX
ST_FUNC int cprime_add_crt(CPRIMEState *s1, const char *filename)
{
  return cprime_add_library_internal(s1, "%s/%s",
                                  filename, AFF_PRINT_ERROR, s1->crt_paths, s1->nb_crt_paths);
}
#endif

// The Library Name Is The Same As The Argument Of The '-L' Option
LIBCPRIMEAPI int cprime_add_library(CPRIMEState *s, const char *libraryname)
{
  static const char * const libs[] =
  {
#if defined CPRIME_TARGET_PE
    "%s/%s.def", "%s/lib%s.def", "%s/%s.dll", "%s/lib%s.dll",
#elif defined CPRIME_TARGET_MACHO
    "%s/lib%s.dylib", "%s/lib%s.tbd",
#elif defined TARGETOS_OpenBSD
    "%s/lib%s.so.*",
#else
    "%s/lib%s.so",
#endif
    "%s/lib%s.a",
    NULL
  };
  int flags = AFF_TYPE_LIB | (s->filetype &AFF_WHOLE_ARCHIVE);
  /* if libraryname begins with a colon, it means search lib paths for
     exactly the following file, without lib prefix or anything */
  if (*libraryname == ':')
    libraryname++;
  else
  {
    const char * const *pp = libs;
    if (s->static_link)
      pp += sizeof(libs) / sizeof(*libs) - 2; // Only "%S/Lib%S.A"
    while (*pp)
    {
      int ret = cprime_add_library_internal(s, *pp,
                                         libraryname, flags, s->library_paths, s->nb_library_paths);
      if (ret != FILE_NOT_FOUND)
        return ret;
      ++pp;
    }
  }
  // Fallback To Try File Without Pre- Or Sufffix
  return cprime_add_dll(s, libraryname, flags | AFF_PRINT_ERROR);
}

// Handle #Pragma Comment(Lib,)
ST_FUNC void cprime_add_pragma_libs(CPRIMEState *s1)
{
  int i;
  for (i = 0; i < s1->nb_pragma_libs; i++)
    cprime_add_library(s1, s1->pragma_libs[i]);
}

//******************************************************
// Options Parser

static int strstart(const char *val, const char **str)
{
  const char *p, *q;
  p = *str;
  q = val;
  while (*q)
  {
    if (*p != *q)
      return 0;
    p++;
    q++;
  }
  *str = p;
  return 1;
}

struct lopt
{
  CPRIMEState *s;
  const char *opt, *arg;
  int match;
};

// Match Linker Option
static int link_option(struct lopt *o, const char *q)
{
  const char *p = o->opt;
  int c;

  // There Should Be 1 Or 2 Dashes
  if (*p++ != '-')
    return 0;
  if (*p == '-')
    p++;
  while ((c = *q) == *p)
  {
    if (c == '\0')
      goto succ; // -Wl,-opt
    ++p;
    if (c == '=')
      goto succ; // -Wl,-opt=arg
    ++q;
  }
  if (c == '=' || c == ':')
  {
    if (*p == '\0')
    {
      if (o->s->link_optind + 1 < o->s->link_argc)
      {
        p = o->s->link_argv[++o->s->link_optind];
        goto succ; // -Wl,-opt,arg
      }
      o->match = 1; // -Wl,-opt -Wl,arg
    }
    else if (c == ':')
      goto succ; // -Wl,-Iarg
  }
  return 0;
succ:
  o->arg = p;
  //printf("set %s '%s'\n", o->opt, o->arg);
  return 1;
}

static void args_parser_add_file(CPRIMEState *s, const char *filename, int filetype);

// Set Linker Options
static int cprime_set_linker(CPRIMEState *s, const char *optarg)
{
  CPRIMEState *s1 = s;

  dynarray_split(&s1->link_argv, &s1->link_argc, optarg, ',');

  while (s->link_optind < s->link_argc)
  {
    char *end = NULL;
    int ignoring = 0;
    struct lopt o = {0};
    o.s = s;
    o.opt = s->link_argv[s->link_optind];

    if (link_option(&o, "Bsymbolic"))
      s->symbolic = 1;
    else if (link_option(&o, "nostdlib"))
      s->nostdlib_paths = 1;
    else if (link_option(&o, "e=") || link_option(&o, "entry="))
      cprime_set_str(&s->elf_entryname, o.arg);
    else if (link_option(&o, "image-base=") || link_option(&o, "Ttext="))
    {
      s->text_addr = strtoull(o.arg, &end, 16);
      s->has_text_addr = 1;
    }
    else if (link_option(&o, "init="))
    {
      cprime_set_str(&s->init_symbol, o.arg);
      ignoring = 1;
    }
    else if (link_option(&o, "fini="))
    {
      cprime_set_str(&s->fini_symbol, o.arg);
      ignoring = 1;
    }
    else if (link_option(&o, "Map="))
    {
      cprime_set_str(&s->mapfile, o.arg);
      ignoring = 1;
    }
    else if (link_option(&o, "oformat="))
    {
#if defined CPRIME_TARGET_PE
      if (0 == strncmp("pe-", o.arg, 3))
#elif PTR_SIZE == 8
      if (0 == strncmp("elf64-", o.arg, 6))
#else
      if (0 == strncmp("elf32-", o.arg, 6))
#endif
        s->output_format = CPRIME_OUTPUT_FORMAT_ELF;
      else if (0 == strcmp("binary", o.arg))
        s->output_format = CPRIME_OUTPUT_FORMAT_BINARY;
#if defined CPRIME_TARGET_COFF
      else if (0 == strcmp("coff", o.arg))
        s->output_format = CPRIME_OUTPUT_FORMAT_COFF;
#endif
      else
        goto err;
    }
    else if (link_option(&o, "export-all-symbols")
             || link_option(&o, "export-dynamic"))
      s->rdynamic = 1;
    else if (link_option(&o, "rpath="))
      cprime_concat_str(&s->rpath, o.arg, ':');
    else if (link_option(&o, "dynamic-linker=") || link_option(&o, "I:"))
      cprime_set_str(&s->elfint, o.arg);
    else if (link_option(&o, "enable-new-dtags"))
      s->enable_new_dtags = 1;
    else if (link_option(&o, "section-alignment="))
      s->section_align = strtoul(o.arg, &end, 16);
    else if (link_option(&o, "soname=") || link_option(&o, "install_name="))
      cprime_set_str(&s->soname, o.arg);
    else if (link_option(&o, "whole-archive"))
      s->filetype |= AFF_WHOLE_ARCHIVE;
    else if (link_option(&o, "no-whole-archive"))
      s->filetype &= ~AFF_WHOLE_ARCHIVE;
    else if (link_option(&o, "znodelete"))
    {
      s->znodelete = 1;
#ifdef CPRIME_TARGET_PE
    }
    else if (link_option(&o, "large-address-aware"))
      s->pe_characteristics |= 0x20;
    else if (link_option(&o, "file-alignment="))
      s->pe_file_align = strtoul(o.arg, &end, 16);
    else if (link_option(&o, "stack="))
      s->pe_stack_size = strtoul(o.arg, &end, 10);
    else if (link_option(&o, "subsystem="))
    {
      if (pe_setsubsy(s, o.arg) < 0)
        goto err;
#elif defined CPRIME_TARGET_MACHO
    }
    else if (link_option(&o, "all_load"))
      s->filetype |= AFF_WHOLE_ARCHIVE;
    else if (link_option(&o, "force_load="))
      args_parser_add_file(s, o.arg, AFF_TYPE_LIB | AFF_WHOLE_ARCHIVE);
    else if (link_option(&o, "single_module"))
    {
      ignoring = 1;
#endif
    }
    else if (link_option(&o, "as-needed"))
      ignoring = 1;
    else if (link_option(&o, "O"))
      ignoring = 1;
    else if (link_option(&o, "z="))
      ignoring = 1;
    else if (link_option(&o, "L:"))
      cprime_add_library_path(s, o.arg);
    else if (link_option(&o, "l:"))
      args_parser_add_file(s, o.arg, AFF_TYPE_LIB | (s->filetype & ~AFF_TYPE_MASK));
    else if (o.match)
    {
      return 0; // expecting argument with next '-Wl,'
    }
    else
    {
err:
      return cprime_error_noabort("unsupported linker option '%s'", o.opt);
    }
    if (ignoring)
      cprime_warning_c(warn_unsupported)("unsupported linker option '%s'", o.opt);
    ++s->link_optind;
  }
  return 0;
}

typedef struct CPRIMEOption
{
  const char *name;
  uint16_t index;
  uint16_t flags;
} CPRIMEOption;

enum
{
  CPRIME_OPTION_ignored = 0,
  CPRIME_OPTION_HELP,
  CPRIME_OPTION_HELP2,
  CPRIME_OPTION_v,
  CPRIME_OPTION_I,
  CPRIME_OPTION_D,
  CPRIME_OPTION_U,
  CPRIME_OPTION_P,
  CPRIME_OPTION_L,
  CPRIME_OPTION_B,
  CPRIME_OPTION_l,
  CPRIME_OPTION_bench,
  CPRIME_OPTION_bt,
  CPRIME_OPTION_b,
  CPRIME_OPTION_g,
  CPRIME_OPTION_c,
  CPRIME_OPTION_dumpmachine,
  CPRIME_OPTION_dumpversion,
  CPRIME_OPTION_d,
  CPRIME_OPTION_static,
  CPRIME_OPTION_std,
  CPRIME_OPTION_shared,
  CPRIME_OPTION_soname,
  CPRIME_OPTION_o,
  CPRIME_OPTION_r,
  CPRIME_OPTION_Wl,
  CPRIME_OPTION_Wp,
  CPRIME_OPTION_W,
  CPRIME_OPTION_O,
  CPRIME_OPTION_mfloat_abi,
  CPRIME_OPTION_m,
  CPRIME_OPTION_f,
  CPRIME_OPTION_isystem,
  CPRIME_OPTION_iwithprefix,
  CPRIME_OPTION_include,
  CPRIME_OPTION_nostdinc,
  CPRIME_OPTION_nostdlib,
  CPRIME_OPTION_print_search_dirs,
  CPRIME_OPTION_rdynamic,
  CPRIME_OPTION_pthread,
  CPRIME_OPTION_run,
  CPRIME_OPTION_rstdin,
  CPRIME_OPTION_w,
  CPRIME_OPTION_E,
  CPRIME_OPTION_M,
  CPRIME_OPTION_MD,
  CPRIME_OPTION_MF,
  CPRIME_OPTION_MM,
  CPRIME_OPTION_MMD,
  CPRIME_OPTION_MP,
  CPRIME_OPTION_x,
  CPRIME_OPTION_ar,
  CPRIME_OPTION_impdef,
  // Macho
  CPRIME_OPTION_dynamiclib,
  CPRIME_OPTION_flat_namespace,
  CPRIME_OPTION_two_levelnamespace,
  CPRIME_OPTION_undefined,
  CPRIME_OPTION_install_name,
  CPRIME_OPTION_compatibility_version,
  CPRIME_OPTION_current_version,
};

#define CPRIME_OPTION_HAS_ARG 0x0001
#define CPRIME_OPTION_NOSEP   0x0002 // Cannot Have Space Before Option And Arg 

/*
 * in cprime_options, if opt-string A is a prefix of opt-string B,
 * it's un-ambiguous if and only if option A is without CPRIME_OPTION_HAS_ARG.
 * otherwise (A with HAS_ARG), if, for instance, A is FOO and B is FOOBAR,
 * then "-FOOBAR" is either A with arg BAR, or B (-FOOBARX too, if B HAS_ARG).
 *
 * cprime_parse_args searches cprime_options in order, so if ambiguous:
 * - if the shorter (A) is earlier: the longer (B) is completely unreachable.
 * - else B wins, and A can't be used with adjacent arg if it also matches B.
 *
 * there are few clashes currently, and the longer is always earlier/reachable.
 * when it's ambiguous, shorter-concat-arg is not useful currently.
 * the sh(1) script 'optclash' can identifiy clashes (cpc root dir, try "-h").
 * at the time of writing, running './optclash' prints this:

    -Wl,... (1642) overrides -W... (1644)
    -Wp,... (1643) overrides -W... (1644)
    -dumpmachine (1630) overrides -d... (1632)
    -dumpversion (1631) overrides -d... (1632)
    -dynamiclib (1623) overrides -d... (1632)
    -flat_namespace (1624) overrides -f... (1650)
    -mfloat-abi... (1647) overrides -m... (1649)

 */
static const CPRIMEOption cprime_options[] =
{
  { "h", CPRIME_OPTION_HELP, 0 },
  { "-help", CPRIME_OPTION_HELP, 0 },
  { "?", CPRIME_OPTION_HELP, 0 },
  { "hh", CPRIME_OPTION_HELP2, 0 },
  { "v", CPRIME_OPTION_v, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "-version", CPRIME_OPTION_v, 0 }, // Handle As Verbose, Also Prints Version
  { "I", CPRIME_OPTION_I, CPRIME_OPTION_HAS_ARG },
  { "D", CPRIME_OPTION_D, CPRIME_OPTION_HAS_ARG },
  { "U", CPRIME_OPTION_U, CPRIME_OPTION_HAS_ARG },
  { "P", CPRIME_OPTION_P, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "L", CPRIME_OPTION_L, CPRIME_OPTION_HAS_ARG },
  { "B", CPRIME_OPTION_B, CPRIME_OPTION_HAS_ARG },
  { "l", CPRIME_OPTION_l, CPRIME_OPTION_HAS_ARG },
  { "bench", CPRIME_OPTION_bench, 0 },
#ifdef CONFIG_CPRIME_BACKTRACE
  { "bt", CPRIME_OPTION_bt, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
#endif
#ifdef CONFIG_CPRIME_BCHECK
  { "b", CPRIME_OPTION_b, 0 },
#endif
  { "g", CPRIME_OPTION_g, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
#ifdef CPRIME_TARGET_MACHO
  { "compatibility_version", CPRIME_OPTION_compatibility_version, CPRIME_OPTION_HAS_ARG },
  { "current_version", CPRIME_OPTION_current_version, CPRIME_OPTION_HAS_ARG },
  { "dynamiclib", CPRIME_OPTION_dynamiclib, 0 },
  { "flat_namespace", CPRIME_OPTION_flat_namespace, 0 },
  { "install_name", CPRIME_OPTION_install_name, CPRIME_OPTION_HAS_ARG },
  { "two_levelnamespace", CPRIME_OPTION_two_levelnamespace, 0 },
  { "undefined", CPRIME_OPTION_undefined, CPRIME_OPTION_HAS_ARG },
#endif
  { "c", CPRIME_OPTION_c, 0 },
  { "dumpmachine", CPRIME_OPTION_dumpmachine, 0},
  { "dumpversion", CPRIME_OPTION_dumpversion, 0},
  { "d", CPRIME_OPTION_d, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "static", CPRIME_OPTION_static, 0 },
  { "std", CPRIME_OPTION_std, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "shared", CPRIME_OPTION_shared, 0 },
  { "soname", CPRIME_OPTION_soname, CPRIME_OPTION_HAS_ARG },
  { "o", CPRIME_OPTION_o, CPRIME_OPTION_HAS_ARG },
  { "pthread", CPRIME_OPTION_pthread, 0},
  { "run", CPRIME_OPTION_run, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "rstdin", CPRIME_OPTION_rstdin, CPRIME_OPTION_HAS_ARG },
  { "rdynamic", CPRIME_OPTION_rdynamic, 0 },
  { "r", CPRIME_OPTION_r, 0 },
  { "Wl,", CPRIME_OPTION_Wl, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "Wp,", CPRIME_OPTION_Wp, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "W", CPRIME_OPTION_W, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "O", CPRIME_OPTION_O, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
#ifdef CPRIME_TARGET_ARM
  { "mfloat-abi", CPRIME_OPTION_mfloat_abi, CPRIME_OPTION_HAS_ARG },
#endif
  { "m", CPRIME_OPTION_m, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "f", CPRIME_OPTION_f, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "isystem", CPRIME_OPTION_isystem, CPRIME_OPTION_HAS_ARG },
  { "include", CPRIME_OPTION_include, CPRIME_OPTION_HAS_ARG },
  { "nostdinc", CPRIME_OPTION_nostdinc, 0 },
  { "nostdlib", CPRIME_OPTION_nostdlib, 0 },
  { "print-search-dirs", CPRIME_OPTION_print_search_dirs, 0 },
  { "w", CPRIME_OPTION_w, 0 },
  { "E", CPRIME_OPTION_E, 0},
  { "M", CPRIME_OPTION_M, 0},
  { "MM", CPRIME_OPTION_MM, 0},
  { "MD", CPRIME_OPTION_MD, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "MMD", CPRIME_OPTION_MMD, CPRIME_OPTION_HAS_ARG | CPRIME_OPTION_NOSEP },
  { "MF", CPRIME_OPTION_MF, CPRIME_OPTION_HAS_ARG },
  { "MP", CPRIME_OPTION_MP, 0},
  { "x", CPRIME_OPTION_x, CPRIME_OPTION_HAS_ARG },
  // Tcctools
  { "ar", CPRIME_OPTION_ar, 0},
#ifdef CPRIME_TARGET_PE
  { "impdef", CPRIME_OPTION_impdef, 0},
#endif
  // ignored (silently, except after -Wunsupported)
  { "arch", 0, CPRIME_OPTION_HAS_ARG},
  { "C", 0, 0 },
  { "-param", 0, CPRIME_OPTION_HAS_ARG },
  { "pedantic", 0, 0 },
  { "pie", 0, 0 },
  { "pipe", 0, 0 },
  { "s", 0, 0 },
  { "traditional", 0, 0 },
  { NULL, 0, 0 },
};

typedef struct FlagDef
{
  uint16_t offset;
  uint16_t flags;
  const char *name;
} FlagDef;

#define WD_ALL    0x0001 // warning is activated when using -Wall 
#define FD_INVERT 0x0002 // Invert Value Before Storing 

static const FlagDef options_W[] =
{
  { offsetof(CPRIMEState, warn_all), WD_ALL, "all" },
  { offsetof(CPRIMEState, warn_error), 0, "error" },
  { offsetof(CPRIMEState, warn_write_strings), 0, "write-strings" },
  { offsetof(CPRIMEState, warn_unsupported), 0, "unsupported" },
  { offsetof(CPRIMEState, warn_implicit_function_declaration), WD_ALL, "implicit-function-declaration" },
  { offsetof(CPRIMEState, warn_discarded_qualifiers), WD_ALL, "discarded-qualifiers" },
  { 0, 0, NULL }
};

static const FlagDef options_f[] =
{
  { offsetof(CPRIMEState, char_is_unsigned), 0, "unsigned-char" },
  { offsetof(CPRIMEState, char_is_unsigned), FD_INVERT, "signed-char" },
  { offsetof(CPRIMEState, nocommon), FD_INVERT, "common" },
  { offsetof(CPRIMEState, leading_underscore), 0, "leading-underscore" },
  { offsetof(CPRIMEState, ms_extensions), 0, "ms-extensions" },
  { offsetof(CPRIMEState, dollars_in_identifiers), 0, "dollars-in-identifiers" },
  { offsetof(CPRIMEState, test_coverage), 0, "test-coverage" },
  { offsetof(CPRIMEState, reverse_funcargs), 0, "reverse-funcargs" },
  { offsetof(CPRIMEState, classic_inline), 0, "classic-inline" },
  { offsetof(CPRIMEState, unwind_tables), 0, "asynchronous-unwind-tables" },
  { 0, 0, NULL }
};

static const FlagDef options_m[] =
{
  { offsetof(CPRIMEState, ms_bitfields), 0, "ms-bitfields" },
#ifdef CPRIME_TARGET_X86_64
  { offsetof(CPRIMEState, nosse), FD_INVERT, "sse" },
#endif
  { 0, 0, NULL }
};

static int set_flag(CPRIMEState *s, const FlagDef *flags, const char *name)
{
  int value, mask, ret;
  const FlagDef *p;
  const char *r;
  unsigned char *f;

  r = name, value = !strstart("no-", &r), mask = 0;

  // when called with options_W, look for -W[no-]error=<option>
  if ((flags->flags & WD_ALL) && strstart("error=", &r))
    value = value ? WARN_ON | WARN_ERR : WARN_NOE, mask = WARN_ON;

  for (ret = -1, p = flags; p->name; ++p)
  {
    if (ret)
    {
      if (strcmp(r, p->name))
        continue;
    }
    else
    {
      if (0 == (p->flags & WD_ALL))
        continue;
    }

    f = (unsigned char *)s + p->offset;
    *f = (*f &mask) | (value ^ !!(p->flags &FD_INVERT));

    if (ret)
    {
      ret = 0;
      if (strcmp(r, "all"))
        break;
    }
  }
  return ret;
}

static const char dumpmachine_str[] =
  // This Is A Best Guess, Please Refine As Necessary
#ifdef CPRIME_TARGET_X86_64
  "x86_64-pc"
#endif
  "-"
#ifdef CPRIME_TARGET_PE
  "mingw32"
#elif defined(CPRIME_TARGET_MACHO)
  "apple-darwin"
#elif TARGETOS_FreeBSD || TARGETOS_FreeBSD_kernel
  "freebsd"
#elif TARGETOS_OpenBSD
  "openbsd"
#elif TARGETOS_NetBSD
  "netbsd"
#elif CONFIG_CPRIME_MUSL
  "linux-musl"
#else
  "linux-gnu"
#endif
  ;

#if defined CPRIME_TARGET_MACHO
static uint32_t parse_version(CPRIMEState *s1, const char *version)
{
  uint32_t a = 0;
  uint32_t b = 0;
  uint32_t c = 0;
  char* last;

  a = strtoul(version, &last, 10);
  if (*last == '.')
  {
    b = strtoul(&last[1], &last, 10);
    if (*last == '.')
      c = strtoul(&last[1], &last, 10);
  }
  if (*last || a > 0xffff || b > 0xff || c > 0xff)
    cprime_error_noabort("version a.b.c not correct: %s", version);
  return (a << 16) | (b << 8) | c;
}
#endif

// Insert Args From 'P' (Separated By Sep Or ' ') Into Argv At Position 'Optind'
static void insert_args(CPRIMEState *s1, char ***pargv, int *pargc, int optind, const char *p, int sep)
{
  int argc = 0;
  char **argv = NULL;
  int i;
  for (i = 0; i < *pargc; ++i)
    if (i == optind)
      dynarray_split(&argv, &argc, p, sep);
    else
      dynarray_add(&argv, &argc, cprime_strdup((*pargv)[i]));
  dynarray_reset(&s1->argv, &s1->argc);
  *pargc = s1->argc = argc;
  *pargv = s1->argv = argv;
}

static void args_parser_add_file(CPRIMEState *s, const char *filename, int filetype)
{
  struct filespec *f = cprime_malloc(sizeof *f + strlen(filename));
  f->type = filetype;
  strcpy(f->name, filename);
  dynarray_add(&s->files, &s->nb_files, f);
  if (filetype & AFF_TYPE_LIB)
    ++s->nb_libraries;
}

/*  parsing is between getopt(3) and getopt_long(3), and permuting-like:
 *  - an option is 1 or more chars.
 *  - at most 1 option per arg in argv.
 *  - an option in argv is "-OPT[...]" (few are --OPT, if OPT is "-...").
 *  - optarg is next arg, or adjacent non-empty (no '='. -std=.. is arg "=..").
 *  - supports also adjacent-only optarg (typically optional).
 *  - supports mixed options and operands ("--" is ignored, except with -run).
 *  - -OPT[...] can be ambiguous, which is resolved using cprime_options's order.
 *    (see cprime_options for details)
 *
 *  specifically, per arg of argv, in order:
 *  - if arg begins with '@' and is not exactly "@": process as @listfile.
 *  - elif arg is exactly "-" or doesn't begin with '-': process as input file.
 *    - if -run... is already set: also stop, arg... become argv of run_main.
 *  - elif arg is "--":
 *    - if -run... is already set: stop, arg... become argv of run_main.
 *    - else ignore it.
 *  - else ("-STRING") try to apply it as option, maybe with next (opt)arg.
 *
 *  after all args, if -run... but no "stop": run_main gets our argv (cpc ...)
 */
// Using * To Argc/Argv To Let "Cpc -Ar" Benefit From @Listfile Expansion
PUB_FUNC int cprime_parse_args(CPRIMEState *s, int *pargc, char ***pargv)
{
  CPRIMEState *s1 = s;
  const CPRIMEOption *popt;
  const char *optarg, *r;
  const char *run = NULL;
  int optind = 1, empty = 1, x;
  char **argv = *pargv;
  int argc = *pargc;

  s->link_optind = s->link_argc;

  while (optind < argc)
  {
    r = argv[optind];
    if (r[0] == '@' && r[1] != '\0')   // Read @Listfile
    {
      int fd; char *p;
      fd = open(++r, O_RDONLY | O_BINARY);
      if (fd < 0)
        return cprime_error_noabort("listfile '%s' not found", r);
      p = cprime_load_text(fd);
      insert_args(s1, &argv, &argc, optind, p, 0);
      close(fd), cprime_free(p);
      continue;
    }
    optind++;
    if (r[0] != '-' || r[1] == '\0')   // File Or '-' (Stdin)
    {
      args_parser_add_file(s, r, s->filetype);
      empty = 0;
dorun:
      if (run)
        break;
      continue;
    }
    // Also allow "cpc <files...> -run -- <args...>"
    if (r[1] == '-' && r[2] == '\0')
      goto dorun;

    // Find Option In Table
    for (popt = cprime_options; ; ++popt)
    {
      const char *p1 = popt->name;
      const char *r1 = r + 1;
      if (p1 == NULL)
        return cprime_error_noabort("invalid option -- '%s'", r);
      if (!strstart(p1, &r1))
        continue;
      optarg = r1;
      if (popt->flags & CPRIME_OPTION_HAS_ARG)
      {
        if (*r1 == '\0' && !(popt->flags & CPRIME_OPTION_NOSEP))
        {
          if (optind >= argc)
            return cprime_error_noabort("argument to '%s' is missing", r);
          optarg = argv[optind++];
        }
      }
      else if (*r1 != '\0')
        continue;
      break;
    }

    switch (popt->index)
    {
    case CPRIME_OPTION_I:
      cprime_add_include_path(s, optarg);
      break;
    case CPRIME_OPTION_D:
      cprime_define_symbol(s, optarg, NULL);
      break;
    case CPRIME_OPTION_U:
      cprime_undefine_symbol(s, optarg);
      break;
    case CPRIME_OPTION_L:
      cprime_add_library_path(s, optarg);
      break;
    case CPRIME_OPTION_B:
      // Set Cpc Utilities Path (Mainly For Cpc Development)
      cprime_set_lib_path(s, optarg);
      continue;
    case CPRIME_OPTION_l:
      args_parser_add_file(s, optarg, AFF_TYPE_LIB | (s->filetype & ~AFF_TYPE_MASK));
      break;
    case CPRIME_OPTION_pthread:
      s->option_pthread = 1;
      break;
    case CPRIME_OPTION_bench:
      s->do_bench = 1;
      break;
#ifdef CONFIG_CPRIME_BACKTRACE
    case CPRIME_OPTION_bt:
      s->rt_num_callers = atoi(optarg); // Zero = Default (6)
      goto enable_backtrace;
enable_backtrace:
      s->do_backtrace = 1;
      if (0 == s->do_debug)
        s->do_debug = 1;
      s->dwarf = CONFIG_DWARF_VERSION;
      break;
#ifdef CONFIG_CPRIME_BCHECK
    case CPRIME_OPTION_b:
      s->do_bounds_check = 1;
      goto enable_backtrace;
#endif
#endif
    case CPRIME_OPTION_g:
      s->do_debug = 2;
      s->dwarf = CONFIG_DWARF_VERSION;
g_redo:
      if (strstart("dwarf", &optarg))
        s->dwarf = (*optarg) ? (0 - atoi(optarg)) : DEFAULT_DWARF_VERSION;
      else if (strcmp("stabs", optarg) == 0)
        return cprime_error_noabort("unsupported option '-gstabs'");
      else if (isnum(*optarg))
      {
        x = *optarg++ - '0';
        // -G0 = No Info, -G1 = Lines/Functions Only, -G2 = Full Info
        s->do_debug = x > 2 ? 2 : x == 0 && s->do_backtrace ? 1 : x;
        goto g_redo;
      }
      break;
    case CPRIME_OPTION_c:
      x = CPRIME_OUTPUT_OBJ;
set_output_type:
      if (s->output_type)
        cprime_warning("-%s: overriding compiler action already specified", popt->name);
      s->output_type = x;
      break;
    case CPRIME_OPTION_d:
      if (*optarg == 'D')
        s->dflag = 3;
      else if (*optarg == 'M')
        s->dflag = 7;
      else if (*optarg == 't')
        s->dflag = 16;
      else if (isnum(*optarg))
        g_debug |= atoi(optarg);
      else
        goto unsupported_option;
      break;
    case CPRIME_OPTION_static:
      s->static_link = 1;
      break;
    case CPRIME_OPTION_std:
      if (strcmp(optarg, "=c11") == 0)
        s->cversion = 201112;
      break;
    case CPRIME_OPTION_shared:
      x = CPRIME_OUTPUT_DLL;
      goto set_output_type;
    case CPRIME_OPTION_soname:
      cprime_set_str(&s->soname, optarg);
      break;
    case CPRIME_OPTION_o:
      if (s->outfile)
        cprime_warning("multiple -o option");
      cprime_set_str(&s->outfile, optarg);
      break;
    case CPRIME_OPTION_r:
      // Generate A .O Merging Several Output Files
      s->option_r = 1;
      x = CPRIME_OUTPUT_OBJ;
      goto set_output_type;
    case CPRIME_OPTION_isystem:
      cprime_add_sysinclude_path(s, optarg);
      break;
    case CPRIME_OPTION_include:
      cstr_printf(&s->cmdline_incl, "#include \"%s\"\n", optarg);
      break;
    case CPRIME_OPTION_nostdinc:
      s->nostdinc = 1;
      break;
    case CPRIME_OPTION_nostdlib:
      s->nostdlib = 1;
      break;
    case CPRIME_OPTION_run:
#ifdef CPRIME_IS_NATIVE
      /* When from script "#!/usr/bin/cpc -run <options>",
         argv[1] is "-run <options>" and argv[2] is <script-name> */
      run = optarg;
      x = CPRIME_OUTPUT_MEMORY;
      goto set_output_type;
#else
      return cprime_error_noabort("-run is not available in a cross compiler");
#endif
#ifdef CPRIME_IS_NATIVE
    case CPRIME_OPTION_rstdin:
      // Custom Stdin For Run_Main
      s->run_stdin = optarg;
      break;
#endif
    case CPRIME_OPTION_v:
      do ++s->verbose; while (*optarg++ == 'v');
      continue;
    case CPRIME_OPTION_f:
      if (set_flag(s, options_f, optarg) < 0)
        goto unsupported_option;
      break;
#ifdef CPRIME_TARGET_ARM
    case CPRIME_OPTION_mfloat_abi:
      // cpc doesn't support soft float yet
      if (!strcmp(optarg, "softfp"))
        s->float_abi = ARM_SOFTFP_FLOAT;
      else if (!strcmp(optarg, "hard"))
        s->float_abi = ARM_HARD_FLOAT;
      else
        return cprime_error_noabort("unsupported float abi '%s'", optarg);
      break;
#endif
    case CPRIME_OPTION_m:
      if (set_flag(s, options_m, optarg) < 0)
      {
        if (x = atoi(optarg), x != 32 && x != 64)
          goto unsupported_option;
        if (PTR_SIZE != x / 8)
          return x;
        continue;
      }
      break;
    case CPRIME_OPTION_W:
      if (optarg[0] && set_flag(s, options_W, optarg) < 0)
        goto unsupported_option;
      break;
    case CPRIME_OPTION_w:
      s->warn_none = 1;
      break;
    case CPRIME_OPTION_rdynamic:
      s->rdynamic = 1;
      break;
    case CPRIME_OPTION_Wl:
      if (cprime_set_linker(s, optarg) < 0)
        return -1;
      break;
    case CPRIME_OPTION_Wp:
      if (argv[0]) // Not With Tcc_Set_Options()
        insert_args(s, &argv, &argc, --optind, optarg, ',');
      break;
    case CPRIME_OPTION_E:
      x = CPRIME_OUTPUT_PREPROCESS;
      goto set_output_type;
    case CPRIME_OPTION_P:
      s->Pflag = atoi(optarg) + 1;
      break;

    case CPRIME_OPTION_M:
      s->include_sys_deps = 1;
    // Fall Through
    case CPRIME_OPTION_MM:
      s->just_deps = 1;
      s->gen_deps = 1;
      if (!s->deps_outfile)
        cprime_set_str(&s->deps_outfile, "-");
      break;
    case CPRIME_OPTION_MD:
      s->include_sys_deps = 1;
    // Fall Through
    case CPRIME_OPTION_MMD:
      s->gen_deps = 1;
      // usually, only "-MMD" is used
      // but the Linux Kernel uses "-MMD,depfile"
      if (*optarg != ',')
        break;
      ++optarg;
    // Fall Through
    case CPRIME_OPTION_MF:
      cprime_set_str(&s->deps_outfile, optarg);
      break;
    case CPRIME_OPTION_MP:
      s->gen_phony_deps = 1;
      break;

    case CPRIME_OPTION_dumpmachine:
      printf("%s\n", dumpmachine_str);
      exit(0);
    case CPRIME_OPTION_dumpversion:
      printf ("%s\n", CPRIME_VERSION);
      exit(0);

    case CPRIME_OPTION_x:
      x = 0;
      if (*optarg == 'c')
        x = AFF_TYPE_C;
      else if (*optarg == 'a')
        x = AFF_TYPE_ASMPP;
      else if (*optarg == 'b')
        x = AFF_TYPE_BIN;
      else if (*optarg == 'n')
        x = AFF_TYPE_NONE;
      else
        cprime_warning("unsupported language '%s'", optarg);
      s->filetype = x | (s->filetype & ~AFF_TYPE_MASK);
      break;
    case CPRIME_OPTION_O:
      s->optimize = isnum(optarg[0]) ? optarg[0] - '0' : 1 /* -O -Os */;
      break;
#if defined CPRIME_TARGET_MACHO
    case CPRIME_OPTION_dynamiclib:
      x = CPRIME_OUTPUT_DLL;
      goto set_output_type;
    case CPRIME_OPTION_flat_namespace:
      break;
    case CPRIME_OPTION_two_levelnamespace:
      break;
    case CPRIME_OPTION_undefined:
      break;
    case CPRIME_OPTION_install_name:
      cprime_set_str(&s->install_name, optarg);
      break;
    case CPRIME_OPTION_compatibility_version:
      s->compatibility_version = parse_version(s, optarg);
      break;
    case CPRIME_OPTION_current_version:
      s->current_version = parse_version(s, optarg);
      break;
#endif
    case CPRIME_OPTION_HELP:
      x = OPT_HELP;
      goto extra_action;
    case CPRIME_OPTION_HELP2:
      x = OPT_HELP2;
      goto extra_action;
    case CPRIME_OPTION_print_search_dirs:
      x = OPT_PRINT_DIRS;
      goto extra_action;
    case CPRIME_OPTION_impdef:
      x = OPT_IMPDEF;
      goto extra_action;
    case CPRIME_OPTION_ar:
      x = OPT_AR;
extra_action:
      if (NULL == argv[0]) // From Tcc_Set_Options()
        return -1;
      if (!empty && x)
        return cprime_error_noabort("cannot parse %s here", r);
      --optind;
      *pargc = argc - optind;
      *pargv = argv + optind;
      return x;
    default:
unsupported_option:
      cprime_warning_c(warn_unsupported)("unsupported option '%s'", r);
      break;
    }
    empty = 0;
  }
  if (s->link_optind < s->link_argc)
    return cprime_error_noabort("argument to '-Wl,%s' is missing", s->link_argv[s->link_optind]);
  if (run)
  {
    if (*run && cprime_set_options(s, run) < 0)
      return -1;
    x = 0;
    goto extra_action;
  }
  if (!empty)
    return 0;
  if (s->verbose == 2)
    return OPT_PRINT_DIRS;
  if (s->verbose)
    return OPT_V;
  return OPT_HELP;
}

LIBCPRIMEAPI int cprime_set_options(CPRIMEState *s, const char *r)
{
  char **argv = NULL;
  int argc = 0, ret;
  dynarray_add(&argv, &argc, 0);
  dynarray_split(&argv, &argc, r, 0);
  ret = cprime_parse_args(s, &argc, &argv);
  dynarray_reset(&argv, &argc);
  return ret;
}

PUB_FUNC void cprime_print_stats(CPRIMEState *s1, unsigned total_time)
{
  if (!total_time)
    total_time = 1;
  fprintf(stderr, "# %d idents, %d lines, %u bytes\n"
          "# %0.3f s, %u lines/s, %0.1f MB/s\n",
          total_idents, total_lines, total_bytes,
          (double)total_time / 1000,
          (unsigned)total_lines * 1000 / total_time,
          (double)total_bytes / 1000 / total_time);
  fprintf(stderr, "# text %u, data.rw %u, data.ro %u, bss %u bytes\n",
          s1->total_output[0],
          s1->total_output[1],
          s1->total_output[2],
          s1->total_output[3]
         );
#ifdef MEM_DEBUG
  fprintf(stderr, "# memory usage");
#ifdef CPRIME_IS_NATIVE
  if (s1->run_size)
  {
    Section *s = s1->symtab;
    unsigned ms = s->data_offset + s->link->data_offset + s->hash->data_offset;
    unsigned rs = s1->run_size;
    fprintf(stderr, ": %d to run, %d symbols, %d other,",
            rs, ms, mem_cur_size - rs - ms);
  }
#endif
  fprintf(stderr, " %d max (bytes)\n", mem_max_size);
#endif
}







