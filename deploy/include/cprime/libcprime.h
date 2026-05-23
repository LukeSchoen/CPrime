#ifndef LIBCPRIME_H
#define LIBCPRIME_H

#ifndef LIBCPRIMEAPI
# define LIBCPRIMEAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void *CPRIMEReallocFunc(void *ptr, unsigned long size);
LIBCPRIMEAPI void cprime_set_realloc(CPRIMEReallocFunc *my_realloc);

typedef struct CPRIMEState CPRIMEState;

LIBCPRIMEAPI CPRIMEState *cprime_new(void);

LIBCPRIMEAPI void cprime_delete(CPRIMEState *s);

LIBCPRIMEAPI void cprime_set_lib_path(CPRIMEState *s, const char *path);

typedef void CPRIMEErrorFunc(void *opaque, const char *msg);
LIBCPRIMEAPI void cprime_set_error_func(CPRIMEState *s, void *error_opaque, CPRIMEErrorFunc *error_func);

LIBCPRIMEAPI int cprime_set_options(CPRIMEState *s, const char *str);

LIBCPRIMEAPI int cprime_add_include_path(CPRIMEState *s, const char *pathname);

LIBCPRIMEAPI int cprime_add_sysinclude_path(CPRIMEState *s, const char *pathname);

LIBCPRIMEAPI void cprime_define_symbol(CPRIMEState *s, const char *sym, const char *value);

LIBCPRIMEAPI void cprime_undefine_symbol(CPRIMEState *s, const char *sym);

LIBCPRIMEAPI int cprime_add_file(CPRIMEState *s, const char *filename);

LIBCPRIMEAPI int cprime_compile_string(CPRIMEState *s, const char *buf);

LIBCPRIMEAPI int cprime_set_output_type(CPRIMEState *s, int output_type);

#define CPRIME_OUTPUT_MEMORY   1
#define CPRIME_OUTPUT_EXE      2
#define CPRIME_OUTPUT_DLL      4
#define CPRIME_OUTPUT_OBJ      3
#define CPRIME_OUTPUT_PREPROCESS 5

LIBCPRIMEAPI int cprime_add_library_path(CPRIMEState *s, const char *pathname);

LIBCPRIMEAPI int cprime_add_library(CPRIMEState *s, const char *libraryname);

LIBCPRIMEAPI int cprime_add_symbol(CPRIMEState *s, const char *name, const void *val);

LIBCPRIMEAPI int cprime_output_file(CPRIMEState *s, const char *filename);

LIBCPRIMEAPI int cprime_run(CPRIMEState *s, int argc, char **argv);

LIBCPRIMEAPI int cprime_relocate(CPRIMEState *s1);

LIBCPRIMEAPI void *cprime_get_symbol(CPRIMEState *s, const char *name);

LIBCPRIMEAPI void cprime_list_symbols(CPRIMEState *s, void *ctx,
    void (*symbol_cb)(void *ctx, const char *name, const void *val));

LIBCPRIMEAPI void *_cprime_setjmp(CPRIMEState *s1, void *jmp_buf, void *top_func, void *longjmp);
#define cprime_setjmp(s1,jb,f) setjmp(_cprime_setjmp(s1, jb, f, longjmp))

LIBCPRIMEAPI int cprime_compile_string_file(CPRIMEState *s, const char *buf, const char *filename);

LIBCPRIMEAPI int elf_output_obj(CPRIMEState *s1, const char *filename);

typedef int CPRIMEBtFunc(void *udata, void *pc, const char *file, int line, const char* func, const char *msg);
LIBCPRIMEAPI void cprime_set_backtrace_func(CPRIMEState *s1, void* userdata, CPRIMEBtFunc*);

#ifdef __cplusplus
}
#endif

#endif



