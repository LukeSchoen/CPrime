/* UCRT termination tables belong to the calling module. Executables use the
   process tables in ucrtbase; DLLs drain their own table before unloading.
   The native CRT's thread-safe static initialization uses the same interface.
   Keep atexit in a separate archive member for the compiler's -run override. */
#ifdef __CPRIME_UCRT__
#include <stdlib.h>

typedef struct {
    _onexit_t *first, *last, *end;
} cpc_onexit_table;

int __cdecl _initialize_onexit_table(cpc_onexit_table *table);
int __cdecl _register_onexit_function(cpc_onexit_table *table, _onexit_t function);
int __cdecl _execute_onexit_table(cpc_onexit_table *table);
int __cdecl _crt_atexit(void (__cdecl *function)(void));
int __cdecl _crt_at_quick_exit(void (__cdecl *function)(void));

static cpc_onexit_table normal_exit_table, quick_exit_table;
static int module_tables_initialized;
/* Before module startup selects its table, a bootstrap compiler's older
   startup has the ordinary executable/process behavior. */
static int use_process_exit_tables = 1;
static int use_run_exit_callbacks;

void __cdecl __cpc_set_onexit_run_mode(void)
{
    use_run_exit_callbacks = 1;
}

_Bool __cdecl __scrt_initialize_onexit_tables(int module_type)
{
    /* This is the native __scrt_module_type ABI: dll=0, exe=1. Startup
       establishes ownership once; a native initializer's DLL fallback must
       not replace an executable's already-initialized process tables. */
    if (module_tables_initialized) return 1;
    if (module_type != 0 && module_type != 1) abort();
    if (!module_type) {
        if (_initialize_onexit_table(&normal_exit_table)
            || _initialize_onexit_table(&quick_exit_table)) return 0;
    }
    use_process_exit_tables = module_type == 1;
    module_tables_initialized = 1;
    return 1;
}

_onexit_t __cdecl _onexit(_onexit_t function)
{
    int result = use_run_exit_callbacks
        ? atexit((void (__cdecl *)(void))function)
        : use_process_exit_tables
        ? _crt_atexit((void (__cdecl *)(void))function)
        : _register_onexit_function(&normal_exit_table, function);
    return result ? 0 : function;
}

int __cdecl at_quick_exit(void (__cdecl *function)(void))
{
    return use_process_exit_tables
        ? _crt_at_quick_exit(function)
        : _register_onexit_function(&quick_exit_table, (_onexit_t)function);
}

void __cdecl __cpc_execute_onexit_table(void)
{
    if (module_tables_initialized && !use_process_exit_tables)
        _execute_onexit_table(&normal_exit_table);
}
#endif
