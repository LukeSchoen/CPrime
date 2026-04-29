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
#define __PRETTY_FUNCTION__ __FUNCTION__
#define __has_builtin(x) 0
#define __has_feature(x) 0
#define __has_attribute(x) 0
#define _Nonnull
#define _Nullable
#define _Nullable_result
#define _Null_unspecified

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

#define __RENAME(X) __asm__(X)
#define __BUILTINBC(ret,name,params) ret __builtin_##name params __RENAME(#name);
#define __BOUND(ret,name,params) ret name params __RENAME(#name);
#define __BOTH(ret,name,params) __BUILTINBC(ret,name,params)__BOUND(ret,name,params)
#define __BUILTIN(ret,name,params) ret __builtin_##name params __RENAME(#name);

__BOTH(void*, memcpy, (void *, const void*, __SIZE_TYPE__))
__BOTH(void*, memmove, (void *, const void*, __SIZE_TYPE__))
__BOTH(void*, memset, (void *, int, __SIZE_TYPE__))
__BOTH(int, memcmp, (const void *, const void*, __SIZE_TYPE__))
__BOTH(__SIZE_TYPE__, strlen, (const char *))
__BOTH(char*, strcpy, (char *, const char *))
__BOTH(char*, strncpy, (char *, const char*, __SIZE_TYPE__))
__BOTH(int, strcmp, (const char*, const char*))
__BOTH(int, strncmp, (const char*, const char*, __SIZE_TYPE__))
__BOTH(char*, strcat, (char*, const char*))
__BOTH(char*, strncat, (char*, const char*, __SIZE_TYPE__))
__BOTH(char*, strchr, (const char*, int))
__BOTH(char*, strrchr, (const char*, int))
__BOTH(char*, strdup, (const char*))

#define __MAYBE_REDIR __BUILTIN
__MAYBE_REDIR(void*, malloc, (__SIZE_TYPE__))
__MAYBE_REDIR(void*, realloc, (void *, __SIZE_TYPE__))
__MAYBE_REDIR(void*, calloc, (__SIZE_TYPE__, __SIZE_TYPE__))
__MAYBE_REDIR(void*, memalign, (__SIZE_TYPE__, __SIZE_TYPE__))
__MAYBE_REDIR(void, free, (void*))
__BOTH(void*, alloca, (__SIZE_TYPE__))
void *alloca(__SIZE_TYPE__);
__BUILTIN(void, abort, (void))
__BOUND(void, longjmp, ())
__BOUND(void*, mmap, ())
__BOUND(int, munmap, ())

#undef __BUILTINBC
#undef __BUILTIN
#undef __BOUND
#undef __BOTH
#undef __MAYBE_REDIR
#undef __RENAME

#define __BUILTIN_EXTERN(name,u) int __builtin_##name(u int); int __builtin_##name##l(u long); int __builtin_##name##ll(u long long);
__BUILTIN_EXTERN(ffs,)
__BUILTIN_EXTERN(clz, unsigned)
__BUILTIN_EXTERN(ctz, unsigned)
__BUILTIN_EXTERN(clrsb,)
__BUILTIN_EXTERN(popcount, unsigned)
__BUILTIN_EXTERN(parity, unsigned)
#undef __BUILTIN_EXTERN
#endif



