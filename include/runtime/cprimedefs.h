#if defined _WIN32
    #if __SIZEOF_POINTER__ == 8
    #define __SIZE_TYPE__ unsigned long long
    #define __PTRDIFF_TYPE__ long long
    #define __LLP64__ 1
    #else
    #define __SIZE_TYPE__ unsigned int
    #define __PTRDIFF_TYPE__ int
    #define __ILP32__ 1
    #endif
    #define __INT64_TYPE__ long long
    #define __SIZEOF_INT__ 4
    #define __INT_MAX__ 0x7fffffff
#if __SIZEOF_LONG__ == 4
    #define __LONG_MAX__ 0x7fffffffL
#else
    #define __LONG_MAX__ 0x7fffffffffffffffL
#endif
    #define __SIZEOF_LONG_LONG__ 8
    #define __LONG_LONG_MAX__ 0x7fffffffffffffffLL
    #define __CHAR_BIT__ 8
    #define __ORDER_LITTLE_ENDIAN__ 1234
    #define __ORDER_BIG_ENDIAN__ 4321
    #define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
    #define __WCHAR_TYPE__ unsigned short
    #define __WINT_TYPE__ unsigned short
    #if __STDC_VERSION__ >= 201112L
    # define __STDC_NO_ATOMICS__ 1
    # define __STDC_NO_COMPLEX__ 1
    # define __STDC_NO_THREADS__ 1
    #endif
#else
#error Windows only
#endif

#define __declspec(x) __attribute__((x))
#define __cdecl
#define __UINTPTR_TYPE__ unsigned __PTRDIFF_TYPE__
#define __INTPTR_TYPE__ __PTRDIFF_TYPE__
#define __INT32_TYPE__ int
#define __assume(x) ((void)0)
#define _Nonnull
#define _Nullable
#define _Nullable_result
#define _Null_unspecified

/* GCC answers the C++/C attribute queries from the same attribute table as
   __has_attribute(), so the aliases keep one source of truth. Both spellings
   are visible in either language mode, as the GCC preprocessor does. */
#define __has_cpp_attribute(x) __has_attribute(x)
#define __has_c_attribute(x) __has_attribute(x)

/* Compile-time execution charset names. */
#define __GNUC_EXECUTION_CHARSET_NAME "UTF-8"
#define __GNUC_WIDE_EXECUTION_CHARSET_NAME "UTF-32LE"


#ifndef __CPRIME_PP__
#define __builtin_offsetof(type, field) ((__SIZE_TYPE__)&((type*)0)->field)
#define __builtin_extract_return_addr(x) x
#define __builtin_huge_val() 1e500
#define __builtin_huge_valf() 1e50f
#define __builtin_huge_vall() 1e5000L
#define __builtin_nanf(ignored_string) (0.0F/0.0F)
#define __builtin_flt_rounds() 1
#define __builtin_bzero(p, ignored_size) bzero(p, sizeof(*(p)))

#if defined __x86_64__
    typedef char *__builtin_va_list;
    #define __builtin_va_arg(ap, t) ((sizeof(t) > 8 || (sizeof(t) & (sizeof(t) - 1))) ? **(t **)((ap += 8) - 8) : *(t  *)((ap += 8) - 8))
#elif defined __arm__
    typedef char *__builtin_va_list;
    #define _cprime_alignof(type) ((int)&((struct {char c;type x;} *)0)->x)
    #define _cprime_align(addr,type) (((unsigned)addr + _cprime_alignof(type) - 1) & ~(_cprime_alignof(type) - 1))
    #define __builtin_va_start(ap,last) (ap = ((char *)&(last)) + ((sizeof(last)+3)&~3))
    #define __builtin_va_arg(ap,type) (ap = (void *) ((_cprime_align(ap,type)+sizeof(type)+3) &~3), *(type *)(ap - ((sizeof(type)+3)&~3)))
#elif defined __aarch64__
    typedef struct { void *__stack; } __builtin_va_list;
#elif defined __riscv
    typedef char *__builtin_va_list;
    #define __va_reg_size (__riscv_xlen >> 3)
    #define _cprime_align(addr,type) (((unsigned long)addr + __alignof__(type) - 1) & -(__alignof__(type)))
    #define __builtin_va_arg(ap,type) (*(sizeof(type) > (2*__va_reg_size) ? *(type **)((ap += __va_reg_size) - __va_reg_size) : (ap = (va_list)(_cprime_align(ap,type) + (sizeof(type)+__va_reg_size - 1)& -__va_reg_size), (type *)(ap - ((sizeof(type)+ __va_reg_size - 1)& -__va_reg_size)))))
#else
    typedef char *__builtin_va_list;
    #define __builtin_va_start(ap,last) (ap = ((char *)&(last)) + ((sizeof(last)+3)&~3))
    #define __builtin_va_arg(ap,t) (*(t*)((ap+=(sizeof(t)+3)&~3)-((sizeof(t)+3)&~3)))
#endif

#define __builtin_va_end(ap) (void)(ap)
#ifndef __builtin_va_copy
#define __builtin_va_copy(dest, src) (dest) = (src)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Direct declarations avoid regenerating the same tokens for every input. */
void* __builtin_memcpy(void *, const void*, __SIZE_TYPE__) __asm__("memcpy");
void* memcpy(void *, const void*, __SIZE_TYPE__) __asm__("memcpy");
void* __builtin_memmove(void *, const void*, __SIZE_TYPE__) __asm__("memmove");
void* memmove(void *, const void*, __SIZE_TYPE__) __asm__("memmove");
void* __builtin_memset(void *, int, __SIZE_TYPE__) __asm__("memset");
void* memset(void *, int, __SIZE_TYPE__) __asm__("memset");
int __builtin_memcmp(const void *, const void*, __SIZE_TYPE__) __asm__("memcmp");
int memcmp(const void *, const void*, __SIZE_TYPE__) __asm__("memcmp");
__SIZE_TYPE__ __builtin_strlen(const char *) __asm__("strlen");
__SIZE_TYPE__ strlen(const char *) __asm__("strlen");
char* __builtin_strcpy(char *, const char *) __asm__("strcpy");
char* strcpy(char *, const char *) __asm__("strcpy");
char* __builtin_strncpy(char *, const char*, __SIZE_TYPE__) __asm__("strncpy");
char* strncpy(char *, const char*, __SIZE_TYPE__) __asm__("strncpy");
int __builtin_strcmp(const char*, const char*) __asm__("strcmp");
int strcmp(const char*, const char*) __asm__("strcmp");
int __builtin_strncmp(const char*, const char*, __SIZE_TYPE__) __asm__("strncmp");
int strncmp(const char*, const char*, __SIZE_TYPE__) __asm__("strncmp");
char* __builtin_strcat(char*, const char*) __asm__("strcat");
char* strcat(char*, const char*) __asm__("strcat");
char* __builtin_strncat(char*, const char*, __SIZE_TYPE__) __asm__("strncat");
char* strncat(char*, const char*, __SIZE_TYPE__) __asm__("strncat");
char* __builtin_strchr(const char*, int) __asm__("strchr");
char* strchr(const char*, int) __asm__("strchr");
char* __builtin_strrchr(const char*, int) __asm__("strrchr");
char* strrchr(const char*, int) __asm__("strrchr");
char* __builtin_strdup(const char*) __asm__("strdup");
char* strdup(const char*) __asm__("strdup");
void* __builtin_malloc(__SIZE_TYPE__) __asm__("malloc");
void* __builtin_realloc(void *, __SIZE_TYPE__) __asm__("realloc");
void* __builtin_calloc(__SIZE_TYPE__, __SIZE_TYPE__) __asm__("calloc");
void* __builtin_memalign(__SIZE_TYPE__, __SIZE_TYPE__) __asm__("memalign");
void __builtin_free(void*) __asm__("free");
void* __builtin_alloca(__SIZE_TYPE__) __asm__("alloca");
void* alloca(__SIZE_TYPE__) __asm__("alloca");
void *alloca(__SIZE_TYPE__);
void __builtin_abort(void) __asm__("abort");
void longjmp() __asm__("longjmp");
void* mmap() __asm__("mmap");
int munmap() __asm__("munmap");
int __builtin_ffs( int);
int __builtin_ffsl( long);
int __builtin_ffsll( long long);
int __builtin_clz(unsigned int);
int __builtin_clzl(unsigned long);
int __builtin_clzll(unsigned long long);
int __builtin_ctz(unsigned int);
int __builtin_ctzl(unsigned long);
int __builtin_ctzll(unsigned long long);
int __builtin_clrsb( int);
int __builtin_clrsbl( long);
int __builtin_clrsbll( long long);
int __builtin_popcount(unsigned int);
int __builtin_popcountl(unsigned long);
int __builtin_popcountll(unsigned long long);
int __builtin_parity(unsigned int);
int __builtin_parityl(unsigned long);
int __builtin_parityll(unsigned long long);

#if defined _WIN32
unsigned char _BitScanForward64(unsigned long *index,
                                unsigned long long mask);
long _InterlockedExchangeAdd(volatile long *target, long value);
void __debugbreak(void);
#endif
#ifdef __cplusplus
}
#endif
#endif



