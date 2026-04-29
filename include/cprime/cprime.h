#ifndef _CPRIME_H
#define _CPRIME_H

#define _DARWIN_C_SOURCE

#define CPRIME_VERSION "0.9.28"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __TINYC__
# undef __attribute__
#endif
#include <string.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <setjmp.h>
#include <time.h>

#ifndef _WIN32
# include <unistd.h>
# include <sys/time.h>
# ifndef CONFIG_CPRIME_STATIC
#  include <dlfcn.h>
# endif

extern float strtof (const char *__nptr, char **__endptr);
extern long double strtold (const char *__nptr, char **__endptr);
#endif

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
# include <io.h>
# include <direct.h>
# include <malloc.h>
# ifndef _MSC_VER
#  include <stdint.h>
# endif
# define inline __inline
# define snprintf _snprintf
# define vsnprintf _vsnprintf
#  define strtold (long double)strtod
#  define strtof (float)strtod
#  define strtoll _strtoi64
#  define strtoull _strtoui64
# ifdef LIBCPRIME_AS_DLL
#  define LIBCPRIMEAPI __declspec(dllexport)
#  define PUB_FUNC LIBCPRIMEAPI
# endif
# ifdef _MSC_VER
#  pragma warning (disable : 4244)
#  pragma warning (disable : 4267)
#  pragma warning (disable : 4996)
#  pragma warning (disable : 4018)
#  pragma warning (disable : 4146)
#  define ssize_t intptr_t
#  ifdef _AMD64_
#   define __x86_64__ 1
#  endif
# endif
# ifndef va_copy
#  define va_copy(a,b) a = b
# endif
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

#ifndef offsetof
#ifdef __clang__
#define offsetof(type, field) __builtin_offsetof(type, field)
#else
#define offsetof(type, field) ((size_t) &((type *)0)->field)
#endif
#endif

#ifndef countof
#define countof(tab) (sizeof(tab) / sizeof((tab)[0]))
#endif

#ifdef _MSC_VER
# define NORETURN __declspec(noreturn)
# define ALIGNED(x) __declspec(align(x))
# define PRINTF_LIKE(x,y)
#else
# define NORETURN __attribute__((noreturn))
# define ALIGNED(x) __attribute__((aligned(x)))
# define PRINTF_LIKE(x,y) __attribute__ ((format (printf, (x), (y))))
#endif

#ifdef _WIN32
# define IS_DIRSEP(c) (c == '/' || c == '\\')
# define IS_ABSPATH(p) (IS_DIRSEP(p[0]) || (p[0] && p[1] == ':' && IS_DIRSEP(p[2])))
# define PATHCMP stricmp
# define PATHSEP ";"
#else
# define IS_DIRSEP(c) (c == '/')
# define IS_ABSPATH(p) IS_DIRSEP(p[0])
# define PATHCMP strcmp
# define PATHSEP ":"
#endif

#if defined(CPRIME_TARGET_ARM) || defined(CPRIME_TARGET_ARM64) || \
    defined(CPRIME_TARGET_C67) || \
    defined(CPRIME_TARGET_RISCV64)
# error only x86_64 is supported
#endif

#if !defined(CPRIME_TARGET_X86_64)
# define CPRIME_TARGET_X86_64 1
# ifdef _WIN32
#  define CPRIME_TARGET_PE 1
# endif
#endif

#if defined _WIN32 == defined CPRIME_TARGET_PE
# if defined __x86_64__ && defined CPRIME_TARGET_X86_64
#  define CPRIME_IS_NATIVE
# endif
#endif

#if defined CONFIG_CPRIME_BACKTRACE && CONFIG_CPRIME_BACKTRACE==0
# undef CONFIG_CPRIME_BACKTRACE
#else
# define CONFIG_CPRIME_BACKTRACE 1
#endif

#if defined CONFIG_CPRIME_BCHECK && CONFIG_CPRIME_BCHECK==0
#  undef CONFIG_CPRIME_BCHECK
#else
#  define CONFIG_CPRIME_BCHECK 1
#endif

#if defined CONFIG_NEW_MACHO && CONFIG_NEW_MACHO==0
#  undef CONFIG_NEW_MACHO
#else
#  define CONFIG_NEW_MACHO 1
#endif

#define TARGETOS_PE 1
#define ELF_OBJ_ONLY
#define CPRIME_USING_DOUBLE_FOR_LDOUBLE 1

#ifdef CONFIG_CPRIME_PIE
# define CONFIG_CPRIME_PIC 1
#endif

#ifndef CONFIG_CPRIME_SEMLOCK
# define CONFIG_CPRIME_SEMLOCK 1
#endif

#ifndef CONFIG_SYSROOT
# define CONFIG_SYSROOT ""
#endif
#if !defined CONFIG_CPRIMEDIR && !defined _WIN32
# define CONFIG_CPRIMEDIR "/usr/local/lib/cpc"
#endif
#ifndef CONFIG_LDDIR
# define CONFIG_LDDIR "lib"
#endif
#ifdef CONFIG_TRIPLET
# define USE_TRIPLET(s) s "/" CONFIG_TRIPLET
# define ALSO_TRIPLET(s) USE_TRIPLET(s) ":" s
#else
# define USE_TRIPLET(s) s
# define ALSO_TRIPLET(s) s
#endif

#ifndef CONFIG_CPRIME_CRTPREFIX
# define CONFIG_CPRIME_CRTPREFIX USE_TRIPLET(CONFIG_SYSROOT "/usr/" CONFIG_LDDIR)
#endif

#ifndef CONFIG_USR_INCLUDE
# define CONFIG_USR_INCLUDE "/usr/include"
#endif

#ifndef CONFIG_CPRIME_SYSINCLUDEPATHS
# if defined CPRIME_TARGET_PE || defined _WIN32
#  define CONFIG_CPRIME_SYSINCLUDEPATHS "{B}/include"PATHSEP"{B}/include/winapi"
# else
#  define CONFIG_CPRIME_SYSINCLUDEPATHS "{B}/include"
# endif
#endif

#ifndef CONFIG_CPRIME_LIBPATHS
# if defined CPRIME_TARGET_PE || defined _WIN32
#  define CONFIG_CPRIME_LIBPATHS "{B}/lib"
# else
#  define CONFIG_CPRIME_LIBPATHS "{B}"
# endif
#endif

#ifndef CONFIG_CPRIME_ELFINTERP
# define CONFIG_CPRIME_ELFINTERP "-"
#endif

#ifndef DEFAULT_ELFINTERP
# define DEFAULT_ELFINTERP(s) CONFIG_CPRIME_ELFINTERP
#endif

#ifndef CPRIME_LIBCPRIME1
# define CPRIME_LIBCPRIME1 "libcprime1.a"
#endif

#if defined CONFIG_USE_LIBGCC && !defined CPRIME_LIBGCC
#define CPRIME_LIBGCC USE_TRIPLET(CONFIG_SYSROOT "/" CONFIG_LDDIR) "/libgcc_s.so.1"
#endif

#ifndef CONFIG_CPRIME_CROSSPREFIX
# define CONFIG_CPRIME_CROSSPREFIX ""
#endif

#include "libcprime.h"
#include "object.h"
#include "dwarf.h"

#ifndef PUB_FUNC
# define PUB_FUNC
#endif

#ifndef ONE_SOURCE
# define ONE_SOURCE 0
#endif

#if ONE_SOURCE
#define ST_INLN static inline
#define ST_FUNC static
#define ST_DATA static
#else
#define ST_INLN
#define ST_FUNC
#define ST_DATA extern
#endif

#ifdef CPRIME_PROFILE
# define static
# define inline
#endif

#define TARGET_DEFS_ONLY
#ifdef CPRIME_TARGET_X86_64
# include "x86_64-gen.c"
# include "x86_64-link.c"
#else
#error unknown target
#endif
#undef TARGET_DEFS_ONLY

#if PTR_SIZE == 8
# define OBJ_CLASSW ELFCLASS64
# define ObjW(type) Obj64_##type
# define ObjW_Rel Obj64_Rela
# define ObjM(type) OBJ64_##type
# define SHT_RELX SHT_RELA
# define REL_SECTION_FMT ".rela%s"
#else
# define OBJ_CLASSW ELFCLASS32
# define ObjW(type) Obj32_##type
# define ObjW_Rel Obj32_Rel
# define ObjM(type) OBJ32_##type
# define SHT_RELX SHT_REL
# define REL_SECTION_FMT ".rel%s"
#endif

#define addr_t ObjW(Addr)
#define ObjSym ObjW(Sym)

#if PTR_SIZE == 8 && !defined CPRIME_TARGET_PE
# define LONG_SIZE 8
#else
# define LONG_SIZE 4
#endif

#define INCLUDE_STACK_SIZE  32
#define IFDEF_STACK_SIZE    64
#define VSTACK_SIZE         512
#define STRING_MAX_SIZE     1024
#define TOKSTR_MAX_SIZE     256
#define PACK_STACK_SIZE     8

#define TOK_HASH_SIZE       16384
#define TOK_ALLOC_INCR      512
#define TOK_MAX_SIZE        4

typedef struct TokenSym {
    struct TokenSym *hash_next;
    struct Sym *sym_define;
    struct Sym *sym_label;
    struct Sym *sym_struct;
    struct Sym *sym_identifier;
    int tok;
    int len;
    char str[1];
} TokenSym;

#ifdef CPRIME_TARGET_PE
typedef unsigned short nwchar_t;
#else
typedef int nwchar_t;
#endif

typedef struct CString {
    int size;
    int size_allocated;
    char *data;
} CString;

typedef struct CType {
    int t;
    struct Sym *ref;
} CType;

typedef union CValue {
    long double ld;
    double d;
    float f;
    uint64_t i;
    struct {
        char *data;
        int size;
    } str;
    int tab[LDOUBLE_SIZE/4];
} CValue;

typedef struct SValue {
    CType type;
    unsigned short r;
    unsigned short r2;
    union {
      struct { int jtrue, jfalse; };
      CValue c;
    };
    union {
      struct { unsigned short cmp_op, cmp_r; };
      struct Sym *sym;
    };

} SValue;

struct SymAttr {
    unsigned
    aligned     : 5,
    packed      : 1,
    weak        : 1,
    visibility  : 2,
    dllexport   : 1,
    nodecorate  : 1,
    dllimport   : 1,
    addrtaken   : 1,
    nodebug     : 1,
    is_class_tag : 1,
    lifecycle_ctor : 1,
    lifecycle_dtor : 1,
    xxxx        : 14;
};

struct FuncAttr {
    unsigned
    func_call   : 3,
    func_type   : 2,
    func_noreturn : 1,
    func_ctor   : 1,
    func_dtor   : 1,
    func_args   : 8,
    func_alwinl : 1,
    xxxx        : 15;
};

typedef struct Sym {
    int v;
    unsigned short r;
    struct SymAttr a;
    union {
        struct {
            int c;
            union {
                int sym_scope;
                int jnext;
                int jind;
                struct FuncAttr f;
                int auxtype;
            };
        };
        long long enum_val;
        int *d;
        struct Sym *cleanup_func;
    };

    CType type;
    union {
        struct Sym *next;
        int *e;
        int asm_label;
        struct Sym *cleanupstate;
        int *vla_array_str;
    };
    struct Sym *prev;
    union {
        struct Sym *prev_tok;
        struct Sym *cleanup_sym;
        struct Sym *cleanup_label;
    };
} Sym;

typedef struct Section {
    unsigned long data_offset;
    unsigned char *data;
    unsigned long data_allocated;
    CPRIMEState *s1;
    int sh_name;
    int sh_num;
    int sh_type;
    int sh_flags;
    int sh_info;
    int sh_addralign;
    int sh_entsize;
    unsigned long sh_size;
    addr_t sh_addr;
    unsigned long sh_offset;
    int nb_hashed_syms;
    struct Section *link;
    struct Section *reloc;
    struct Section *hash;
    struct Section *prev;
    char name[1];
} Section;

typedef struct DLLReference {
    int level;
    void *handle;
    unsigned char found, index;
    char name[1];
} DLLReference;

#define SYM_STRUCT     0x40000000
#define SYM_FIELD      0x20000000
#define SYM_FIRST_ANOM 0x10000000

#define FUNC_NEW       1
#define FUNC_OLD       2
#define FUNC_ELLIPSIS  3

#define FUNC_CDECL     0
#define FUNC_STDCALL   1
#define FUNC_FASTCALL1 2
#define FUNC_FASTCALL2 3
#define FUNC_FASTCALL3 4
#define FUNC_FASTCALLW 5
#define FUNC_THISCALL  6

#define MACRO_OBJ      0
#define MACRO_FUNC     1
#define MACRO_JOIN     2

#define LABEL_DEFINED  0
#define LABEL_FORWARD  1
#define LABEL_DECLARED 2
#define LABEL_GONE     3

#define TYPE_ABSTRACT  1
#define TYPE_DIRECT    2
#define TYPE_PARAM     4
#define TYPE_NEST      8
#define TYPE_LOCAL_CTOR_INIT 16

#define IO_BUF_SIZE 8192

typedef struct BufferedFile {
    uint8_t *buf_ptr;
    uint8_t *buf_end;
    int fd;
    struct BufferedFile *prev;
    int line_num;
    int line_ref;
    int ifndef_macro;
    int ifndef_macro_saved;
    int *ifdef_stack_ptr;
    int include_next_index;
    int prev_tok_flags;
    char filename[1024];
    char *true_filename;
    unsigned char unget[4];
    unsigned char buffer[1];
} BufferedFile;

#define CH_EOB   '\\'
#define CH_EOF   (-1)

typedef struct TokenString {
    int *str;
    int len;
    int need_spc;
    int allocated_len;
    int last_line_num;
    int save_line_num;

    struct TokenString *prev;
    const int *prev_ptr;
    char alloc;
} TokenString;

typedef struct AttributeDef {
    struct SymAttr a;
    struct FuncAttr f;
    struct Section *section;
    Sym *cleanup_func;
    int alias_target;
    int asm_label;
    char attr_mode;
} AttributeDef;

typedef struct InlineFunc {
    TokenString *func_str;
    Sym *sym;
    char filename[1];
} InlineFunc;

typedef struct CachedInclude {
    int ifndef_macro;
    int once;
    int hash_next;
    char filename[1];
} CachedInclude;

#define CACHED_INCLUDES_HASH_SIZE 32

#ifdef CONFIG_CPRIME_ASM
typedef struct ExprValue {
    uint64_t v;
    Sym *sym;
    int pcrel;
} ExprValue;

#define MAX_ASM_OPERANDS 30
typedef struct ASMOperand {
    int id;
    char constraint[16];
    char asm_str[16];
    SValue *vt;
    int ref_index;
    int input_index;
    int priority;
    int reg;
    int is_llong;
    int is_memory;
    int is_rw;
    int is_label;
} ASMOperand;
#endif

struct sym_attr {
    unsigned got_offset;
    unsigned plt_offset;
    int plt_sym;
    int dyn_index;
#ifdef CPRIME_TARGET_ARM
    unsigned char plt_thumb_stub:1;
#endif
};

struct CPRIMEState {
    unsigned char verbose;
    unsigned char nostdinc;
    unsigned char nostdlib;
    unsigned char nostdlib_paths;
    unsigned char nocommon;
    unsigned char static_link;
    unsigned char rdynamic;
    unsigned char symbolic;
    unsigned char znodelete;
    unsigned char filetype;
    unsigned char optimize;
    unsigned char option_pthread;
    unsigned char enable_new_dtags;
    unsigned int  cversion;

    unsigned char char_is_unsigned;
    unsigned char leading_underscore;
    unsigned char ms_extensions;
    unsigned char dollars_in_identifiers;
    unsigned char ms_bitfields;
    unsigned char reverse_funcargs;
    unsigned char classic_inline;
    unsigned char unwind_tables;

    unsigned char warn_none;
    unsigned char warn_all;
    unsigned char warn_error;
    unsigned char warn_write_strings;
    unsigned char warn_unsupported;
    unsigned char warn_implicit_function_declaration;
    unsigned char warn_discarded_qualifiers;
    #define WARN_ON  1
    unsigned char warn_num;

    unsigned char option_r;
    unsigned char do_bench;
    unsigned char just_deps;
    unsigned char gen_deps;
    unsigned char include_sys_deps;
    unsigned char gen_phony_deps;

    unsigned char do_debug;
    unsigned char dwarf;
    unsigned char do_backtrace;
#ifdef CONFIG_CPRIME_BCHECK

    unsigned char do_bounds_check;
#endif
    unsigned char test_coverage;

    unsigned char gnu_ext;

    unsigned char cprime_ext;

    unsigned char dflag;
    unsigned char Pflag;

#ifdef CPRIME_TARGET_X86_64
    unsigned char nosse;
#endif
#ifdef CPRIME_TARGET_ARM
    unsigned char float_abi;
#endif

    unsigned char has_text_addr;
    addr_t text_addr;
    unsigned section_align;
#ifdef CPRIME_TARGET_I386
    int seg_size;
#endif

    char *cprime_lib_path;
    char *soname;
    char *rpath;
    char *elfint;
    char *elf_entryname;
    char *init_symbol;
    char *fini_symbol;
    char *mapfile;

    int output_type;

    int output_format;

    int run_test;

    DLLReference **loaded_dlls;
    int nb_loaded_dlls;

    char **include_paths;
    int nb_include_paths;

    char **sysinclude_paths;
    int nb_sysinclude_paths;

    char **library_paths;
    int nb_library_paths;

    char **crt_paths;
    int nb_crt_paths;

    CString cmdline_defs;

    CString cmdline_incl;

    void *error_opaque;
    void (*error_func)(void *opaque, const char *msg);
    int error_set_jmp_enabled;
    jmp_buf error_jmp_buf;
    int nb_errors;

    FILE *ppfp;

    char **target_deps;
    int nb_target_deps;

    BufferedFile *include_stack[INCLUDE_STACK_SIZE];
    BufferedFile **include_stack_ptr;

    int ifdef_stack[IFDEF_STACK_SIZE];
    int *ifdef_stack_ptr;

    int cached_includes_hash[CACHED_INCLUDES_HASH_SIZE];
    CachedInclude **cached_includes;
    int nb_cached_includes;

    int pack_stack[PACK_STACK_SIZE];
    int *pack_stack_ptr;
    char **pragma_libs;
    int nb_pragma_libs;

    struct InlineFunc **inline_fns;
    int nb_inline_fns;

    Section **sections;
    int nb_sections;

    Section **priv_sections;
    int nb_priv_sections;

    Section *text_section, *data_section, *rodata_section, *bss_section;
    Section *common_section;
    Section *cur_text_section;
#ifdef CONFIG_CPRIME_BCHECK

    Section *bounds_section;
    Section *lbounds_section;
#endif

    union { Section *symtab_section, *symtab; };

    Section *dynsymtab_section;

    Section *dynsym;

    Section *got, *plt;

    Section *eh_frame_section;
    Section *eh_frame_hdr_section;
    unsigned long eh_start;

    Section *dwarf_info_section;
    Section *dwarf_abbrev_section;
    Section *dwarf_line_section;
    Section *dwarf_aranges_section;
    Section *dwarf_str_section;
    Section *dwarf_line_str_section;
    int dwlo, dwhi;

    Section *tcov_section;

    struct _cprimedbg *dState;

    struct sym_attr *sym_attrs;
    int nb_sym_attrs;

    ObjW_Rel *qrel;
    #define qrel s1->qrel

#ifdef CPRIME_TARGET_RISCV64
    struct pcrel_hi { addr_t addr, val; } last_hi;
    struct pcrel_hi *pcrel_hi_entries;
    int nb_pcrel_hi_entries;
    int alloc_pcrel_hi_entries;
    #define last_hi s1->last_hi
#endif

#ifdef CPRIME_TARGET_PE

    int pe_subsystem;
    unsigned pe_characteristics;
    unsigned pe_file_align;
    unsigned pe_stack_size;
    addr_t pe_imagebase;
# ifdef CPRIME_TARGET_X86_64
    Section *uw_pdata;
    int uw_sym;
    unsigned uw_offs;
# endif
#endif

#if defined CPRIME_TARGET_MACHO
    char *install_name;
    uint32_t compatibility_version;
    uint32_t current_version;
#endif

#ifndef ELF_OBJ_ONLY
    int nb_sym_versions;
    struct sym_version *sym_versions;
    int nb_sym_to_version;
    int *sym_to_version;
    int dt_verneednum;
    Section *versym_section;
    Section *verneed_section;
#endif

#ifdef CPRIME_IS_NATIVE
    const char *run_main;
    void *run_ptr;
    unsigned run_size;
    const char *run_stdin;
#ifdef _WIN64
    void *run_function_table;
#endif
    struct CPRIMEState *next;
    struct rt_context *rc;
    void *run_lj, *run_jb;
    CPRIMEBtFunc *bt_func;
    void *bt_data;
#endif

#ifdef CONFIG_CPRIME_BACKTRACE
    int rt_num_callers;
#endif

    int total_idents;
    int total_lines;
    unsigned int total_bytes;
    unsigned int total_output[4];

    unsigned char *ld_p;

    const char *current_filename;

    struct filespec **files;
    int nb_files;
    int nb_libraries;
    char *outfile;
    char *deps_outfile;
    int argc;
    char **argv;

    char **link_argv;
    int link_argc, link_optind;
};

struct filespec {
    char type;
    char name[1];
};

#define VT_VALMASK   0x003f
#define VT_CONST     0x0030
#define VT_LLOCAL    0x0031
#define VT_LOCAL     0x0032
#define VT_CMP       0x0033
#define VT_JMP       0x0034
#define VT_JMPI      0x0035
#define VT_LVAL      0x0100
#define VT_SYM       0x0200
#define VT_MUSTCAST  0x0C00
#define VT_NONCONST  0x1000
#define VT_MUSTBOUND 0x4000
#define VT_BOUNDED   0x8000

#define VT_BTYPE       0x000f
#define VT_VOID             0
#define VT_BYTE             1
#define VT_SHORT            2
#define VT_INT              3
#define VT_LLONG            4
#define VT_PTR              5
#define VT_FUNC             6
#define VT_STRUCT           7
#define VT_FLOAT            8
#define VT_DOUBLE           9
#define VT_LDOUBLE         10
#define VT_BOOL            11
#define VT_QLONG           13
#define VT_QFLOAT          14

#define VT_UNSIGNED    0x0010
#define VT_DEFSIGN     0x0020
#define VT_ARRAY       0x0040
#define VT_BITFIELD    0x0080
#define VT_CONSTANT    0x0100
#define VT_VOLATILE    0x0200
#define VT_VLA         0x0400
#define VT_LONG        0x0800

#define VT_EXTERN  0x00001000
#define VT_STATIC  0x00002000
#define VT_TYPEDEF 0x00004000
#define VT_INLINE  0x00008000

#define VT_STRUCT_SHIFT 20
#define VT_STRUCT_MASK (((1U << (6+6)) - 1) << VT_STRUCT_SHIFT | VT_BITFIELD)
#define BIT_POS(t) (((t) >> VT_STRUCT_SHIFT) & 0x3f)
#define BIT_SIZE(t) (((t) >> (VT_STRUCT_SHIFT + 6)) & 0x3f)

#define VT_UNION    (1 << VT_STRUCT_SHIFT | VT_STRUCT)
#define VT_ENUM     (2 << VT_STRUCT_SHIFT)
#define VT_ENUM_VAL (3 << VT_STRUCT_SHIFT)

#define IS_ENUM(t) ((t & VT_STRUCT_MASK) == VT_ENUM)
#define IS_ENUM_VAL(t) ((t & VT_STRUCT_MASK) == VT_ENUM_VAL)
#define IS_UNION(t) ((t & (VT_STRUCT_MASK|VT_BTYPE)) == VT_UNION)

#define VT_ATOMIC   VT_VOLATILE

#define VT_STORAGE (VT_EXTERN | VT_STATIC | VT_TYPEDEF | VT_INLINE)
#define VT_TYPE (~(VT_STORAGE|VT_STRUCT_MASK))

#define VT_ASM (VT_VOID | 4 << VT_STRUCT_SHIFT)
#define VT_ASM_FUNC (VT_VOID | 5 << VT_STRUCT_SHIFT)
#define IS_ASM_SYM(sym) (((sym)->type.t & ((VT_BTYPE|VT_STRUCT_MASK) & ~(1<<VT_STRUCT_SHIFT))) == VT_ASM)
#define IS_ASM_FUNC(t) ((t & (VT_BTYPE|VT_STRUCT_MASK)) == VT_ASM_FUNC)

#define VT_BT_ARRAY (6 << VT_STRUCT_SHIFT)
#define IS_BT_ARRAY(t) ((t & VT_STRUCT_MASK) == VT_BT_ARRAY)

#define BFVAL(M,N) ((unsigned)((M) & ~((M) << 1)) * (N))
#define BFGET(X,M) (((X) & (M)) / BFVAL(M,1))
#define BFSET(X,M,N) ((X) = ((X) & ~(M)) | BFVAL(M,N))

#define TOK_LAND  0x90
#define TOK_LOR   0x91

#define TOK_ULT 0x92
#define TOK_UGE 0x93
#define TOK_EQ  0x94
#define TOK_NE  0x95
#define TOK_ULE 0x96
#define TOK_UGT 0x97
#define TOK_Nset 0x98
#define TOK_Nclear 0x99
#define TOK_LT  0x9c
#define TOK_GE  0x9d
#define TOK_LE  0x9e
#define TOK_GT  0x9f

#define TOK_ISCOND(t) (t >= TOK_LAND && t <= TOK_GT)

#define TOK_DEC     0x80
#define TOK_MID     0x81
#define TOK_INC     0x82
#define TOK_UDIV    0x83
#define TOK_UMOD    0x84
#define TOK_PDIV    0x85
#define TOK_UMULL   0x86
#define TOK_ADDC1   0x87
#define TOK_ADDC2   0x88
#define TOK_SUBC1   0x89
#define TOK_SUBC2   0x8a
#define TOK_SHL     '<'
#define TOK_SAR     '>'
#define TOK_SHR     0x8b
#define TOK_NEG     TOK_MID

#define TOK_ARROW   0xa0
#define TOK_DOTS    0xa1
#define TOK_TWODOTS 0xa2
#define TOK_TWOSHARPS 0xa3
#define TOK_PLCHLDR 0xa4
#define TOK_PPJOIN  (TOK_TWOSHARPS | SYM_FIELD)
#define TOK_SOTYPE  0xa7

#define TOK_A_ADD   0xb0
#define TOK_A_SUB   0xb1
#define TOK_A_MUL   0xb2
#define TOK_A_DIV   0xb3
#define TOK_A_MOD   0xb4
#define TOK_A_AND   0xb5
#define TOK_A_OR    0xb6
#define TOK_A_XOR   0xb7
#define TOK_A_SHL   0xb8
#define TOK_A_SAR   0xb9

#define TOK_ASSIGN(t) (t >= TOK_A_ADD && t <= TOK_A_SAR)
#define TOK_ASSIGN_OP(t) ("+-*/%&|^<>"[t - TOK_A_ADD])

#define TOK_CCHAR   0xc0
#define TOK_LCHAR   0xc1
#define TOK_CINT    0xc2
#define TOK_CUINT   0xc3
#define TOK_CLLONG  0xc4
#define TOK_CULLONG 0xc5
#define TOK_CLONG   0xc6
#define TOK_CULONG  0xc7
#define TOK_STR     0xc8
#define TOK_LSTR    0xc9
#define TOK_CFLOAT  0xca
#define TOK_CDOUBLE 0xcb
#define TOK_CLDOUBLE 0xcc
#define TOK_PPNUM   0xcd
#define TOK_PPSTR   0xce
#define TOK_LINENUM 0xcf

#define TOK_HAS_VALUE(t) (t >= TOK_CCHAR && t <= TOK_LINENUM)

#define TOK_EOF       (-1)
#define TOK_LINEFEED  10

#define TOK_IDENT 256

enum cprime_token {
    TOK_LAST = TOK_IDENT - 1
#define DEF(id, str) ,id
#include "cprimetok.h"
#undef DEF
};

#define TOK_UIDENT TOK_DEFINE

ST_DATA struct CPRIMEState *cprime_state;
ST_DATA void** stk_data;
ST_DATA int nb_stk_data;
ST_DATA int g_debug;

ST_FUNC char *pstrcpy(char *buf, size_t buf_size, const char *s);
ST_FUNC char *pstrcat(char *buf, size_t buf_size, const char *s);
ST_FUNC char *pstrncpy(char *out, size_t buf_size, const char *s, size_t num);
PUB_FUNC char *cprime_basename(const char *name);
PUB_FUNC char *cprime_fileextension (const char *name);

PUB_FUNC void cprime_free(void *ptr);
PUB_FUNC void *cprime_malloc(unsigned long size);
PUB_FUNC void *cprime_mallocz(unsigned long size);
PUB_FUNC void *cprime_realloc(void *ptr, unsigned long size);
PUB_FUNC char *cprime_strdup(const char *str);

#ifdef MEM_DEBUG
#define cprime_free(ptr)           cprime_free_debug(ptr)
#define cprime_malloc(size)        cprime_malloc_debug(size, __FILE__, __LINE__)
#define cprime_mallocz(size)       cprime_mallocz_debug(size, __FILE__, __LINE__)
#define cprime_realloc(ptr,size)   cprime_realloc_debug(ptr, size, __FILE__, __LINE__)
#define cprime_strdup(str)         cprime_strdup_debug(str, __FILE__, __LINE__)
PUB_FUNC void cprime_free_debug(void *ptr);
PUB_FUNC void *cprime_malloc_debug(unsigned long size, const char *file, int line);
PUB_FUNC void *cprime_mallocz_debug(unsigned long size, const char *file, int line);
PUB_FUNC void *cprime_realloc_debug(void *ptr, unsigned long size, const char *file, int line);
PUB_FUNC char *cprime_strdup_debug(const char *str, const char *file, int line);
#endif

ST_FUNC void libc_free(void *ptr);

#define free(p) use_cprime_free(p)
#define malloc(s) use_cprime_malloc(s)
#define realloc(p, s) use_cprime_realloc(p, s)
#undef strdup
#define strdup(s) use_cprime_strdup(s)
PUB_FUNC int _cprime_error_noabort(const char *fmt, ...) PRINTF_LIKE(1,2);
PUB_FUNC NORETURN void _cprime_error(const char *fmt, ...) PRINTF_LIKE(1,2);
PUB_FUNC void _cprime_warning(const char *fmt, ...) PRINTF_LIKE(1,2);
#define cprime_internal_error(msg) \
    cprime_error("internal compiler error in %s:%d: %s", __FUNCTION__,__LINE__,msg)

ST_FUNC void dynarray_add(void *ptab, int *nb_ptr, void *data);
ST_FUNC void dynarray_reset(void *pp, int *n);
ST_INLN void cstr_ccat(CString *cstr, int ch);
ST_FUNC void cstr_cat(CString *cstr, const char *str, int len);
ST_FUNC void cstr_wccat(CString *cstr, int ch);
ST_FUNC void cstr_new(CString *cstr);
ST_FUNC void cstr_free(CString *cstr);
ST_FUNC int cstr_printf(CString *cs, const char *fmt, ...) PRINTF_LIKE(2,3);
ST_FUNC int cstr_vprintf(CString *cstr, const char *fmt, va_list ap);
ST_FUNC void cstr_reset(CString *cstr);
ST_FUNC void cprime_open_bf(CPRIMEState *s1, const char *filename, int initlen);
ST_FUNC int cprime_open(CPRIMEState *s1, const char *filename);
ST_FUNC void cprime_close(void);

#define stk_push(p) dynarray_add(&stk_data, &nb_stk_data, p)
#define stk_pop() (--nb_stk_data)

#define cstr_new_s(cstr) (cstr_new(cstr), stk_push(&(cstr)->data))
#define cstr_free_s(cstr) (cstr_free(cstr), stk_pop())

ST_FUNC int cprime_add_file_internal(CPRIMEState *s1, const char *filename, int flags);

#define AFF_PRINT_ERROR     0x10
#define AFF_REFERENCED_DLL  0x20
#define AFF_TYPE_BIN        0x40
#define AFF_WHOLE_ARCHIVE   0x80

#define AFF_TYPE_NONE   0
#define AFF_TYPE_C      1
#define AFF_TYPE_ASM    2
#define AFF_TYPE_ASMPP  4
#define AFF_TYPE_LIB    8
#define AFF_TYPE_MASK   (7 | AFF_TYPE_BIN)

#define AFF_BINTYPE_REL 1
#define AFF_BINTYPE_DYN 2
#define AFF_BINTYPE_AR  3
#define AFF_BINTYPE_C67 4

#define FILE_NOT_FOUND -2
#define FILE_NOT_RECOGNIZED -3

#ifndef ELF_OBJ_ONLY
ST_FUNC int cprime_add_crt(CPRIMEState *s, const char *filename);
#endif
ST_FUNC int cprime_add_dll(CPRIMEState *s, const char *filename, int flags);
ST_FUNC int cprime_add_support(CPRIMEState *s1, const char *filename);
#ifdef CONFIG_CPRIME_BCHECK
ST_FUNC void cprime_add_bcheck(CPRIMEState *s1);
#endif
#ifdef CONFIG_CPRIME_BACKTRACE
ST_FUNC void cprime_add_btstub(CPRIMEState *s1);
#endif
ST_FUNC void cprime_add_pragma_libs(CPRIMEState *s1);
PUB_FUNC int cprime_add_library_err(CPRIMEState *s, const char *f);
PUB_FUNC void cprime_print_stats(CPRIMEState *s, unsigned total_time);
PUB_FUNC int cprime_parse_args(CPRIMEState *s, int *argc, char ***argv);
#ifdef _WIN32
ST_FUNC char *normalize_slashes(char *path);
#endif
ST_FUNC DLLReference *cprime_add_dllref(CPRIMEState *s1, const char *dllname, int level);
ST_FUNC char *cprime_load_text(int fd);

ST_FUNC int normalized_PATHCMP(const char *f1, const char *f2);

#define OPT_HELP 1
#define OPT_HELP2 2
#define OPT_V 3
#define OPT_PRINT_DIRS 4
#define OPT_AR 5
#define OPT_IMPDEF 6
#define OPT_M32 32
#define OPT_M64 64

ST_DATA struct BufferedFile *file;
ST_DATA int tok;
ST_DATA CValue tokc;
ST_DATA const int *macro_ptr;
ST_DATA int parse_flags;
ST_DATA int tok_flags;
ST_DATA CString tokcstr;

ST_DATA int tok_ident;
ST_DATA TokenSym **table_ident;
ST_DATA int pp_expr;

#define TOK_FLAG_BOL   0x0001
#define TOK_FLAG_BOF   0x0002
#define TOK_FLAG_ENDIF 0x0004

#define PARSE_FLAG_PREPROCESS 0x0001
#define PARSE_FLAG_TOK_NUM    0x0002
#define PARSE_FLAG_LINEFEED   0x0004
#define PARSE_FLAG_ASM_FILE 0x0008
#define PARSE_FLAG_SPACES     0x0010
#define PARSE_FLAG_ACCEPT_STRAYS 0x0020
#define PARSE_FLAG_TOK_STR    0x0040

#define IS_SPC 1
#define IS_ID  2
#define IS_NUM 4

enum line_macro_output_format {
    LINE_MACRO_OUTPUT_FORMAT_GCC,
    LINE_MACRO_OUTPUT_FORMAT_NONE,
    LINE_MACRO_OUTPUT_FORMAT_STD,
    LINE_MACRO_OUTPUT_FORMAT_P10 = 11
};

ST_FUNC TokenSym *tok_alloc(const char *str, int len);
ST_FUNC int tok_alloc_const(const char *str);
ST_FUNC const char *get_tok_str(int v, CValue *cv);
ST_FUNC void begin_macro(TokenString *str, int alloc);
ST_FUNC void end_macro(void);
ST_FUNC int set_idnum(int c, int val);
ST_INLN void tok_str_new(TokenString *s);
ST_FUNC TokenString *tok_str_alloc(void);
ST_FUNC void tok_str_free(TokenString *s);
ST_FUNC void tok_str_free_str(int *str);
ST_FUNC void tok_str_add(TokenString *s, int t);
ST_FUNC void tok_str_add_tok(TokenString *s);
ST_INLN void define_push(int v, int macro_type, int *str, Sym *first_arg);
ST_FUNC void define_undef(Sym *s);
ST_INLN Sym *define_find(int v);
ST_FUNC void free_defines(Sym *b);
ST_FUNC void parse_define(void);
ST_FUNC void skip_to_eol(int warn);
ST_FUNC void preprocess(int is_bof);
ST_FUNC void next(void);
ST_INLN void unget_tok(int last_tok);
ST_FUNC void preprocess_start(CPRIMEState *s1, int filetype);
ST_FUNC void preprocess_end(CPRIMEState *s1);
ST_FUNC void cprimepp_new(CPRIMEState *s);
ST_FUNC void cprimepp_delete(CPRIMEState *s);
ST_FUNC void cprimepp_putfile(const char *filename);
ST_FUNC int cprime_preprocess(CPRIMEState *s1);
ST_FUNC void skip(int c);
ST_FUNC NORETURN void expect(const char *msg);
ST_FUNC void pp_error(CString *cs);

static inline int is_space(int ch) {
    return ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f' || ch == '\r';
}
static inline int isid(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static inline int isnum(int c) {
    return c >= '0' && c <= '9';
}
static inline int isoct(int c) {
    return c >= '0' && c <= '7';
}
static inline int toup(int c) {
    return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c;
}

#define SYM_POOL_NB (8192 / sizeof(Sym))

ST_DATA Sym *global_stack;
ST_DATA Sym *local_stack;
ST_DATA Sym *local_label_stack;
ST_DATA Sym *global_label_stack;
ST_DATA Sym *define_stack;
ST_DATA CType int_type, func_old_type, char_pointer_type;
ST_DATA SValue *vtop;
ST_DATA int rsym, anon_sym, ind, loc;
ST_DATA char debug_modes;

ST_DATA int nocode_wanted;
ST_DATA int global_expr;
ST_DATA CType func_vt;
ST_DATA int func_var;
ST_DATA int func_vc;
ST_DATA int func_ind;
ST_DATA const char *funcname;

ST_FUNC void cprimegen_init(CPRIMEState *s1);
ST_FUNC int cprimegen_compile(CPRIMEState *s1);
ST_FUNC void cprimegen_finish(CPRIMEState *s1);
ST_FUNC void check_vstack(void);

ST_INLN int is_float(int t);
ST_FUNC int ieee_finite(double d);
ST_FUNC int exact_log2p1(int i);
ST_FUNC void test_lvalue(void);

ST_FUNC ObjSym *elfsym(Sym *);
ST_FUNC void update_storage(Sym *sym);
ST_FUNC void put_extern_sym2(Sym *sym, int sh_num, addr_t value, unsigned long size, int can_add_underscore);
ST_FUNC void put_extern_sym(Sym *sym, Section *section, addr_t value, unsigned long size);
#if PTR_SIZE == 4
ST_FUNC void greloc(Section *s, Sym *sym, unsigned long offset, int type);
#endif
ST_FUNC void greloca(Section *s, Sym *sym, unsigned long offset, int type, addr_t addend);

ST_INLN void sym_free(Sym *sym);
ST_FUNC Sym *sym_push(int v, CType *type, int r, int c);
ST_FUNC void sym_pop(Sym **ptop, Sym *b, int keep);
ST_FUNC Sym *sym_push2(Sym **ps, int v, int t, int c);
ST_FUNC Sym *sym_find2(Sym *s, int v);
ST_INLN Sym *sym_find(int v);
ST_FUNC Sym *label_find(int v);
ST_FUNC Sym *label_push(Sym **ptop, int v, int flags);
ST_FUNC void label_pop(Sym **ptop, Sym *slast, int keep);
ST_INLN Sym *struct_find(int v);

ST_FUNC Sym *global_identifier_push(int v, int t, int c);
ST_FUNC Sym *external_global_sym(int v, CType *type);
ST_FUNC Sym *external_helper_sym(int v);
ST_FUNC void vpush_helper_func(int v);
ST_FUNC void vset(CType *type, int r, int v);
ST_FUNC void vset_VT_CMP(int op);
ST_FUNC void vpushi(int v);
ST_FUNC void vpushv(SValue *v);
ST_FUNC void vpushsym(CType *type, Sym *sym);
ST_FUNC void vswap(void);
ST_FUNC void vrott(int n);
ST_FUNC void vrotb(int n);
ST_FUNC void vrev(int n);
ST_FUNC void vpop(void);
#if PTR_SIZE == 4
ST_FUNC void lexpand(void);
#endif
#ifdef CPRIME_TARGET_ARM
ST_FUNC int get_reg_ex(int rc, int rc2);
#endif
ST_FUNC void save_reg(int r);
ST_FUNC void save_reg_upstack(int r, int n);
ST_FUNC int get_reg(int rc);
ST_FUNC void save_regs(int n);
ST_FUNC void gaddrof(void);
ST_FUNC int gv(int rc);
ST_FUNC void gv2(int rc1, int rc2);
ST_FUNC void gen_op(int op);
ST_FUNC int type_size(CType *type, int *a);
ST_FUNC void mk_pointer(CType *type);
ST_FUNC void vstore(void);
ST_FUNC void inc(int post, int c);
ST_FUNC CString* parse_mult_str(const char *msg);
ST_FUNC CString* parse_asm_str(void);
ST_FUNC void indir(void);
ST_FUNC void unary(void);
ST_FUNC void gexpr(void);
ST_FUNC int expr_const(void);
#if defined CONFIG_CPRIME_BCHECK || defined CPRIME_TARGET_C67
ST_FUNC Sym *get_sym_ref(CType *type, Section *sec, unsigned long offset, unsigned long size);
#endif
#if defined CPRIME_TARGET_X86_64 && !defined CPRIME_TARGET_PE
ST_FUNC int classify_x86_64_va_arg(CType *ty);
#endif
#ifdef CONFIG_CPRIME_BCHECK
ST_FUNC void gbound_args(int nb_args);
ST_DATA int func_bound_add_epilog;
#endif
ST_FUNC Sym *gfunc_set_param(Sym *s, int c, int byref);

#define CPRIME_OUTPUT_FORMAT_ELF    0
#define CPRIME_OUTPUT_FORMAT_BINARY 1
#define CPRIME_OUTPUT_FORMAT_COFF   2
#define CPRIME_OUTPUT_DYN           CPRIME_OUTPUT_DLL

#define ARMAG  "!<arch>\n"

ST_FUNC void cprimeelf_new(CPRIMEState *s);
ST_FUNC void cprimeelf_delete(CPRIMEState *s);
ST_FUNC void cprimeelf_begin_file(CPRIMEState *s1);
ST_FUNC void cprimeelf_end_file(CPRIMEState *s1);
ST_FUNC Section *new_section(CPRIMEState *s1, const char *name, int sh_type, int sh_flags);
ST_FUNC void section_realloc(Section *sec, unsigned long new_size);
ST_FUNC size_t section_add(Section *sec, addr_t size, int align);
ST_FUNC void *section_ptr_add(Section *sec, addr_t size);
ST_FUNC Section *find_section(CPRIMEState *s1, const char *name);
ST_FUNC void free_section(Section *s);
ST_FUNC Section *new_symtab(CPRIMEState *s1, const char *symtab_name, int sh_type, int sh_flags, const char *strtab_name, const char *hash_name, int hash_sh_flags);
ST_FUNC void init_symtab(Section *s);

ST_FUNC int put_elf_str(Section *s, const char *sym);
ST_FUNC int put_elf_sym(Section *s, addr_t value, unsigned long size, int info, int other, int shndx, const char *name);
ST_FUNC int set_elf_sym(Section *s, addr_t value, unsigned long size, int info, int other, int shndx, const char *name);
ST_FUNC int find_elf_sym(Section *s, const char *name);
ST_FUNC void put_elf_reloc(Section *symtab, Section *s, unsigned long offset, int type, int symbol);
ST_FUNC void put_elf_reloca(Section *symtab, Section *s, unsigned long offset, int type, int symbol, addr_t addend);

ST_FUNC void resolve_common_syms(CPRIMEState *s1);
ST_FUNC void relocate_syms(CPRIMEState *s1, Section *symtab, int do_resolve);
ST_FUNC void relocate_sections(CPRIMEState *s1);

ST_FUNC ssize_t full_read(int fd, void *buf, size_t count);
ST_FUNC void *load_data(int fd, unsigned long file_offset, unsigned long size);
ST_FUNC int cprime_object_type(int fd, ObjW(Ehdr) *h);
ST_FUNC int cprime_load_object_file(CPRIMEState *s1, int fd, unsigned long file_offset);
ST_FUNC int cprime_load_archive(CPRIMEState *s1, int fd, int alacarte);
ST_FUNC void add_array(CPRIMEState *s1, const char *sec, int c);

ST_FUNC struct sym_attr *get_sym_attr(CPRIMEState *s1, int index, int alloc);
ST_FUNC addr_t get_sym_addr(CPRIMEState *s, const char *name, int err, int forc);
ST_FUNC void list_elf_symbols(CPRIMEState *s, void *ctx,
    void (*symbol_cb)(void *ctx, const char *name, const void *val));
ST_FUNC int set_global_sym(CPRIMEState *s1, const char *name, Section *sec, addr_t offs);

#define for_each_elem(sec, startoff, elem, type) \
    for (elem = (type *) sec->data + startoff; \
         elem < (type *) (sec->data + sec->data_offset); elem++)

#ifndef ELF_OBJ_ONLY
ST_FUNC int cprime_load_dll(CPRIMEState *s1, int fd, const char *filename, int level);
ST_FUNC int cprime_load_ldscript(CPRIMEState *s1, int fd);
ST_FUNC void cprimeelf_add_crtbegin(CPRIMEState *s1);
ST_FUNC void cprimeelf_add_crtend(CPRIMEState *s1);
#endif
#if 1
ST_FUNC void cprime_debug_stabn(CPRIMEState *s1, int type, int value);
#endif

#ifndef CPRIME_TARGET_PE
ST_FUNC void cprime_add_runtime(CPRIMEState *s1);
#endif

#ifndef CPRIME_TARGET_PE
ST_FUNC int code_reloc (int reloc_type);
ST_FUNC int gotplt_entry_type (int reloc_type);

enum gotplt_entry {
    NO_GOTPLT_ENTRY,
    BUILD_GOT_ONLY,
    AUTO_GOTPLT_ENTRY,
    ALWAYS_GOTPLT_ENTRY
};
#define NEED_RELOC_TYPE
#if !defined CPRIME_TARGET_MACHO || defined CPRIME_IS_NATIVE
ST_FUNC unsigned create_plt_entry(CPRIMEState *s1, unsigned got_offset, struct sym_attr *attr);
ST_FUNC void relocate_plt(CPRIMEState *s1);
ST_FUNC void build_got_entries(CPRIMEState *s1, int got_sym);
#define NEED_BUILD_GOT
#endif
#endif

ST_FUNC void relocate(CPRIMEState *s1, ObjW_Rel *rel, int type, unsigned char *ptr, addr_t addr, addr_t val);

ST_DATA const char * const target_machine_defs;
ST_DATA const int reg_classes[NB_REGS];

ST_FUNC void gsym_addr(int t, int a);
ST_FUNC void gsym(int t);
ST_FUNC void load(int r, SValue *sv);
ST_FUNC void store(int r, SValue *v);
ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret, int *align, int *regsize);
ST_FUNC void gfunc_call(int nb_args);
ST_FUNC void gfunc_prolog(Sym *func_sym);
ST_FUNC void gfunc_epilog(void);
ST_FUNC void gen_fill_nops(int);
ST_FUNC int gjmp(int t);
ST_FUNC void gjmp_addr(int a);
ST_FUNC int gjmp_cond(int op, int t);
ST_FUNC int gjmp_append(int n, int t);
ST_FUNC void gen_opi(int op);
ST_FUNC void gen_opf(int op);
ST_FUNC void gen_cvt_ftoi(int t);
ST_FUNC void gen_cvt_itof(int t);
ST_FUNC void gen_cvt_ftof(int t);
ST_FUNC void ggoto(void);
#ifndef CPRIME_TARGET_C67
ST_FUNC void o(unsigned int c);
#endif
ST_FUNC void gen_vla_sp_save(int addr);
ST_FUNC void gen_vla_sp_restore(int addr);
ST_FUNC void gen_vla_alloc(CType *type, int align);

static inline uint16_t read16le(unsigned char *p) {
    return p[0] | (uint16_t)p[1] << 8;
}
static inline void write16le(unsigned char *p, uint16_t x) {
    p[0] = x & 255;  p[1] = x >> 8 & 255;
}
static inline uint32_t read32le(unsigned char *p) {
  return read16le(p) | (uint32_t)read16le(p + 2) << 16;
}
static inline void write32le(unsigned char *p, uint32_t x) {
    write16le(p, x);  write16le(p + 2, x >> 16);
}
static inline void add32le(unsigned char *p, int32_t x) {
    write32le(p, read32le(p) + x);
}
static inline uint64_t read64le(unsigned char *p) {
  return read32le(p) | (uint64_t)read32le(p + 4) << 32;
}
static inline void write64le(unsigned char *p, uint64_t x) {
    write32le(p, x);  write32le(p + 4, x >> 32);
}
static inline void add64le(unsigned char *p, int64_t x) {
    write64le(p, read64le(p) + x);
}

#if defined CPRIME_TARGET_X86_64 || defined CPRIME_TARGET_ARM
ST_FUNC void g(int c);
ST_FUNC void gen_le16(int c);
ST_FUNC void gen_le32(int c);
#endif
#if defined CPRIME_TARGET_X86_64
ST_FUNC void gen_addr32(int r, Sym *sym, int c);
ST_FUNC void gen_addrpc32(int r, Sym *sym, int c);
ST_FUNC void gen_cvt_csti(int t);
ST_FUNC void gen_increment_tcov (SValue *sv);
#endif

#ifdef CPRIME_TARGET_X86_64
ST_FUNC void gen_addr64(int r, Sym *sym, int64_t c);
ST_FUNC void gen_opl(int op);
#ifdef CPRIME_TARGET_PE
ST_FUNC void gen_vla_result(int addr);
#endif
ST_FUNC void gen_cvt_sxtw(void);
ST_FUNC void gen_cvt_csti(int t);
#endif

#ifdef CPRIME_TARGET_ARM
#if defined(CPRIME_ARM_EABI) && !defined(CONFIG_CPRIME_ELFINTERP)
PUB_FUNC const char *default_elfinterp(struct CPRIMEState *s);
#endif
ST_FUNC void arm_init(struct CPRIMEState *s);
ST_FUNC void gen_increment_tcov (SValue *sv);
#endif

#ifdef CPRIME_TARGET_ARM64
ST_FUNC void gen_opl(int op);
ST_FUNC void gfunc_return(CType *func_type);
ST_FUNC void gen_va_start(void);
ST_FUNC void gen_va_arg(CType *t);
ST_FUNC void gen_clear_cache(void);
ST_FUNC void gen_cvt_sxtw(void);
ST_FUNC void gen_cvt_csti(int t);
ST_FUNC void gen_increment_tcov (SValue *sv);
#endif

#ifdef CPRIME_TARGET_RISCV64
ST_FUNC void gen_opl(int op);

ST_FUNC void gen_va_start(void);
ST_FUNC void arch_transfer_ret_regs(int);
ST_FUNC void gen_cvt_sxtw(void);
ST_FUNC void gen_increment_tcov (SValue *sv);
#endif

#ifdef CPRIME_TARGET_C67
#endif

#ifdef CPRIME_TARGET_COFF
ST_FUNC int cprime_output_coff(CPRIMEState *s1, FILE *f);
ST_FUNC int cprime_load_coff(CPRIMEState * s1, int fd);
#endif

ST_FUNC void asm_instr(void);
ST_FUNC void asm_global_instr(void);
ST_FUNC int cprime_assemble(CPRIMEState *s1, int do_preprocess);
#ifdef CONFIG_CPRIME_ASM
ST_FUNC int find_constraint(ASMOperand *operands, int nb_operands, const char *name, const char **pp);
ST_FUNC Sym* get_asm_sym(int name, Sym *csym);
ST_FUNC void asm_expr(CPRIMEState *s1, ExprValue *pe);
ST_FUNC int asm_int_expr(CPRIMEState *s1);

ST_FUNC void gen_expr32(ExprValue *pe);
#ifdef CPRIME_TARGET_X86_64
ST_FUNC void gen_expr64(ExprValue *pe);
#endif
ST_FUNC void asm_opcode(CPRIMEState *s1, int opcode);
ST_FUNC int asm_parse_regvar(int t);
ST_FUNC void asm_compute_constraints(ASMOperand *operands, int nb_operands, int nb_outputs, const uint8_t *clobber_regs, int *pout_reg);
ST_FUNC void subst_asm_operand(CString *add_str, SValue *sv, int modifier);
ST_FUNC void asm_gen_code(ASMOperand *operands, int nb_operands, int nb_outputs, int is_output, uint8_t *clobber_regs, int out_reg);
ST_FUNC void asm_clobber(uint8_t *clobber_regs, const char *str);
#endif

#ifdef CPRIME_TARGET_PE
ST_FUNC int pe_load_file(struct CPRIMEState *s1, int fd, const char *filename);
ST_FUNC int pe_output_file(CPRIMEState * s1, const char *filename);
ST_FUNC int pe_putimport(CPRIMEState *s1, int dllindex, const char *name, addr_t value);
ST_FUNC int pe_setsubsy(CPRIMEState *s1, const char *arg);
#ifdef CPRIME_TARGET_X86_64
ST_FUNC void pe_add_unwind_data(unsigned start, unsigned end, unsigned stack);
#endif
PUB_FUNC int cprime_get_dllexports(const char *filename, char **pp);

# define ST_PE_EXPORT 0x10
# define ST_PE_IMPORT 0x20
# define ST_PE_STDCALL 0x40
#endif
#define ST_ASM_SET 0x04

#ifdef CPRIME_TARGET_MACHO
ST_FUNC int macho_output_file(CPRIMEState * s1, const char *filename);
ST_FUNC int macho_load_dll(CPRIMEState *s1, int fd, const char *filename, int lev);
ST_FUNC int macho_load_tbd(CPRIMEState *s1, int fd, const char *filename, int lev);
#ifdef CPRIME_IS_NATIVE
ST_FUNC void cprime_add_macos_sdkpath(CPRIMEState* s);
ST_FUNC char* macho_tbd_soname(int fd);
#endif
#endif

#ifdef CPRIME_IS_NATIVE
#ifdef CONFIG_CPRIME_STATIC
#define RTLD_LAZY       0x001
#define RTLD_NOW        0x002
#define RTLD_GLOBAL     0x100
#define RTLD_DEFAULT    NULL

ST_FUNC void *dlopen(const char *filename, int flag);
ST_FUNC void dlclose(void *p);
ST_FUNC const char *dlerror(void);
ST_FUNC void *dlsym(void *handle, const char *symbol);
#endif
ST_FUNC void cprime_run_free(CPRIMEState *s1);
#endif

#if 0
ST_FUNC int cprime_tool_ar(CPRIMEState *s, int argc, char **argv);
#ifdef CPRIME_TARGET_PE
ST_FUNC int cprime_tool_impdef(CPRIMEState *s, int argc, char **argv);
#endif
ST_FUNC int cprime_tool_cross(CPRIMEState *s, char **argv, int option);
ST_FUNC int gen_makedeps(CPRIMEState *s, const char *target, const char *filename);
#endif

ST_FUNC void cprime_debug_new(CPRIMEState *s);

ST_FUNC void cprime_debug_start(CPRIMEState *s1);
ST_FUNC void cprime_debug_end(CPRIMEState *s1);
ST_FUNC void cprime_debug_bincl(CPRIMEState *s1);
ST_FUNC void cprime_debug_eincl(CPRIMEState *s1);
ST_FUNC void cprime_debug_newfile(CPRIMEState *s1);

ST_FUNC void cprime_debug_line(CPRIMEState *s1);
ST_FUNC void cprime_add_debug_info(CPRIMEState *s1, Sym *s, Sym *e);
ST_FUNC void cprime_debug_funcstart(CPRIMEState *s1, Sym *sym);
ST_FUNC void cprime_debug_prolog_epilog(CPRIMEState *s1, int value);
ST_FUNC void cprime_debug_funcend(CPRIMEState *s1, int size);
ST_FUNC void cprime_debug_extern_sym(CPRIMEState *s1, Sym *sym, int sh_num, int sym_bind, int sym_type);
ST_FUNC void cprime_debug_typedef(CPRIMEState *s1, Sym *sym);
ST_FUNC void cprime_debug_fix_forw(CPRIMEState *s1, CType *t);

#if !(defined ELF_OBJ_ONLY || defined CPRIME_TARGET_ARM || defined TARGETOS_BSD)
ST_FUNC void cprime_eh_frame_start(CPRIMEState *s1);
ST_FUNC void cprime_eh_frame_end(CPRIMEState *s1);
ST_FUNC void cprime_eh_frame_hdr(CPRIMEState *s1, int final);
#define CPRIME_EH_FRAME 1
#endif

ST_FUNC void cprime_tcov_start(CPRIMEState *s1);
ST_FUNC void cprime_tcov_end(CPRIMEState *s1);
ST_FUNC void cprime_tcov_check_line(CPRIMEState *s1, int start);
ST_FUNC void cprime_tcov_block_end(CPRIMEState *s1, int line);
ST_FUNC void cprime_tcov_block_begin(CPRIMEState *s1);
ST_FUNC void cprime_tcov_reset_ind(CPRIMEState *s1);

#define tcov_section            s1->tcov_section
#define eh_frame_section        s1->eh_frame_section
#define eh_frame_hdr_section    s1->eh_frame_hdr_section
#define dwarf_info_section      s1->dwarf_info_section
#define dwarf_abbrev_section    s1->dwarf_abbrev_section
#define dwarf_line_section      s1->dwarf_line_section
#define dwarf_aranges_section   s1->dwarf_aranges_section
#define dwarf_str_section       s1->dwarf_str_section
#define dwarf_line_str_section  s1->dwarf_line_str_section

#define DWARF_MAX_128	((8 * sizeof (int64_t) + 6) / 7)
#define	dwarf_read_1(ln,end) \
	((ln) < (end) ? *(ln)++ : 0)
#define	dwarf_read_2(ln,end) \
	((ln) + 1 < (end) ? read16le(((ln)+=2) - 2) : 0)
#define	dwarf_read_4(ln,end) \
	((ln) + 3 < (end) ? read32le(((ln)+=4) - 4) : 0)
#define	dwarf_read_8(ln,end) \
	((ln) + 7 < (end) ? read64le(((ln)+=8) - 8) : 0)
static inline uint64_t
dwarf_read_uleb128(unsigned char **ln, unsigned char *end)
{
    unsigned char *cp = *ln;
    uint64_t retval = 0;
    int i;
    for (i = 0; i < DWARF_MAX_128; i++) {
	uint64_t byte = dwarf_read_1(cp, end);
        retval |= (byte & 0x7f) << (i * 7);
	if ((byte & 0x80) == 0)
	    break;
    }
    *ln = cp;
    return retval;
}
static inline int64_t
dwarf_read_sleb128(unsigned char **ln, unsigned char *end)
{
    unsigned char *cp = *ln;
    int64_t retval = 0;
    int i;
    for (i = 0; i < DWARF_MAX_128; i++) {
	uint64_t byte = dwarf_read_1(cp, end);
        retval |= (byte & 0x7f) << (i * 7);
	if ((byte & 0x80) == 0) {
	    if ((byte & 0x40) && (i + 1) * 7 < 64)
		retval |= (uint64_t)-1LL << ((i + 1) * 7);
	    break;
	}
    }
    *ln = cp;
    return retval;
}

#ifdef CPRIME_TARGET_MACHO
# define DEFAULT_DWARF_VERSION 2
#else
# define DEFAULT_DWARF_VERSION 5
#endif

#ifndef CONFIG_DWARF_VERSION
# define CONFIG_DWARF_VERSION 0
#endif

#if defined CPRIME_TARGET_X86_64
# define R_DATA_32DW R_X86_64_32
#else
# define R_DATA_32DW R_DATA_32
#endif

#if CONFIG_CPRIME_SEMLOCK
#if defined _WIN32
typedef struct { int init; CRITICAL_SECTION cs; } CPRIMESem;
static inline void wait_sem(CPRIMESem *p) {
    if (!p->init)
        InitializeCriticalSection(&p->cs), p->init = 1;
    EnterCriticalSection(&p->cs);
}
static inline void post_sem(CPRIMESem *p) {
    LeaveCriticalSection(&p->cs);
}
#else
#include <semaphore.h>
typedef struct { int init; sem_t sem; } CPRIMESem;
static inline void wait_sem(CPRIMESem *p) {
    if (!p->init)
        sem_init(&p->sem, 0, 1), p->init = 1;
    while (sem_wait(&p->sem) < 0 && errno == EINTR);
}
static inline void post_sem(CPRIMESem *p) {
    sem_post(&p->sem);
}
#endif
#define CPRIME_SEM(s) CPRIMESem s
#define WAIT_SEM wait_sem
#define POST_SEM post_sem
#else
#define CPRIME_SEM(s)
#define WAIT_SEM(p)
#define POST_SEM(p)
#endif

#undef ST_DATA
#if ONE_SOURCE
#define ST_DATA static
#else
#define ST_DATA
#endif

#define text_section        CPRIME_STATE_VAR(text_section)
#define data_section        CPRIME_STATE_VAR(data_section)
#define rodata_section      CPRIME_STATE_VAR(rodata_section)
#define bss_section         CPRIME_STATE_VAR(bss_section)
#define common_section      CPRIME_STATE_VAR(common_section)
#define cur_text_section    CPRIME_STATE_VAR(cur_text_section)
#define bounds_section      CPRIME_STATE_VAR(bounds_section)
#define lbounds_section     CPRIME_STATE_VAR(lbounds_section)
#define symtab_section      CPRIME_STATE_VAR(symtab_section)
#define gnu_ext             CPRIME_STATE_VAR(gnu_ext)
#define cprime_error_noabort   CPRIME_SET_STATE(_cprime_error_noabort)
#define cprime_error           CPRIME_SET_STATE(_cprime_error)
#define cprime_warning         CPRIME_SET_STATE(_cprime_warning)

#define total_idents        CPRIME_STATE_VAR(total_idents)
#define total_lines         CPRIME_STATE_VAR(total_lines)
#define total_bytes         CPRIME_STATE_VAR(total_bytes)

PUB_FUNC void cprime_enter_state(CPRIMEState *s1);
PUB_FUNC void cprime_exit_state(CPRIMEState *s1);

#define cprime_warning_c(sw) CPRIME_SET_STATE((\
    cprime_state->warn_num = offsetof(CPRIMEState, sw) \
    - offsetof(CPRIMEState, warn_none), _cprime_warning))

#endif

#undef CPRIME_STATE_VAR
#undef CPRIME_SET_STATE

#ifdef USING_GLOBALS
# define CPRIME_STATE_VAR(sym) cprime_state->sym
# define CPRIME_SET_STATE(fn) fn
# undef USING_GLOBALS
# undef _cprime_error
#else
# define CPRIME_STATE_VAR(sym) s1->sym
# define CPRIME_SET_STATE(fn) (cprime_enter_state(s1),fn)
# define _cprime_error use_cprime_error_noabort
#endif



