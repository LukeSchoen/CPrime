//+---------------------------------------------------------------------------

#include <stdlib.h>

#ifdef __leading_underscore
# define _(s) s
#else
# define _(s) _##s
#endif

extern void (*_(_init_array_start)[]) (int argc, _TCHAR **argv, _TCHAR **envp);
extern void (*_(_init_array_end)[]) (int argc, _TCHAR **argv, _TCHAR **envp);
extern void (*_(_fini_array_start)[]) (void);
extern void (*_(_fini_array_end)[]) (void);
extern void __cpc_run_static_destructors(void);
#ifdef __CPRIME_NATIVE_CRT__
extern int (*__cpc_native_xi_start[])(void), (*__cpc_native_xi_end[])(void);
extern void (*__cpc_native_xc_start[])(void), (*__cpc_native_xc_end[])(void);
extern void (*__cpc_native_xp_start[])(void), (*__cpc_native_xp_end[])(void);
extern void (*__cpc_native_xt_start[])(void), (*__cpc_native_xt_end[])(void);
typedef void (WINAPI *native_tls_callback)(void *, DWORD, void *);
extern native_tls_callback __cpc_native_tls_init_callback;
extern native_tls_callback __cpc_native_tls_dtor_callback;
static int cpc_runtime_in_memory;
#ifdef __CPRIME_UCRT__
_Bool __cdecl __scrt_initialize_onexit_tables(int);
void __cdecl __cpc_execute_onexit_table(void);
void __cdecl __cpc_set_onexit_run_mode(void);
#endif

static void run_native_terminators(void)
{
    void (**terminator)(void);
    for (terminator = __cpc_native_xp_start;
         terminator != __cpc_native_xp_end; ++terminator)
        if (*terminator) (*terminator)();
    for (terminator = __cpc_native_xt_start;
         terminator != __cpc_native_xt_end; ++terminator)
        if (*terminator) (*terminator)();
}

#ifndef CPC_RUNTIME_DLL
#ifdef __CPRIME_UCRT__
void __cdecl _register_thread_local_exe_atexit_callback(native_tls_callback);
#endif
static void run_native_tls_destructors(void)
{
    __cpc_native_tls_dtor_callback(0, DLL_PROCESS_DETACH, 0);
}
#endif
#endif /* __CPRIME_NATIVE_CRT__ */

#if defined(__CPRIME_UCRT__) && defined(CPC_WINDOWS_CRT_STARTUP_H)
static void initialize_arguments(int wildcard)
{
#ifdef _UNICODE
    if (_configure_wide_argv(wildcard ? 2 : 1) || _initialize_wide_environment())
#else
    if (_configure_narrow_argv(wildcard ? 2 : 1) || _initialize_narrow_environment())
#endif
        exit(255);
}
#endif

static void run_ctors(int argc, _TCHAR **argv, _TCHAR **env)
{
#ifdef __CPRIME_NATIVE_CRT__
    int (**c_initializer)(void);
    void (**cpp_initializer)(void);
#endif
    int i = 0;
#ifdef __CPRIME_NATIVE_CRT__
#ifdef __CPRIME_UCRT__
    if (cpc_runtime_in_memory) __cpc_set_onexit_run_mode();
#ifdef CPC_RUNTIME_DLL
    if (!__scrt_initialize_onexit_tables(0)) exit(255);
#else
    if (!__scrt_initialize_onexit_tables(cpc_runtime_in_memory ? 0 : 1)) exit(255);
#endif
#endif
#ifndef CPC_RUNTIME_DLL
    /* Register first so native atexit callbacks finish before XP/XT tables. */
    atexit(run_native_terminators);
#endif
    for (c_initializer = __cpc_native_xi_start;
         c_initializer != __cpc_native_xi_end; ++c_initializer)
        if (*c_initializer && (*c_initializer)())
            exit(255);
    for (cpp_initializer = __cpc_native_xc_start;
         cpp_initializer != __cpc_native_xc_end; ++cpp_initializer)
        if (*cpp_initializer) (*cpp_initializer)();
#endif
    while (&_(_init_array_start)[i] != _(_init_array_end))
        (*_(_init_array_start)[i++])(argc, argv, env);
#ifdef __CPRIME_NATIVE_CRT__
    /* The loader's process-attach notification precedes CRT initialization.
       Native dynamic TLS therefore needs this explicit primary-thread call. */
    if (__cpc_native_tls_init_callback)
        __cpc_native_tls_init_callback(0, DLL_THREAD_ATTACH, 0);
#ifndef CPC_RUNTIME_DLL
    if (__cpc_native_tls_dtor_callback)
#ifdef __CPRIME_UCRT__
    {
        if (cpc_runtime_in_memory)
            atexit(run_native_tls_destructors);
        else
            _register_thread_local_exe_atexit_callback(__cpc_native_tls_dtor_callback);
    }
#else
        atexit(run_native_tls_destructors);
#endif
#endif
#endif /* __CPRIME_NATIVE_CRT__ */
}

static void run_dtors(void)
{
    int i = 0;
#if !defined(__CPRIME_NATIVE_CRT__) || !defined(__CPRIME_UCRT__)
    __cpc_run_static_destructors();
#endif
    while (&_(_fini_array_end)[i] != _(_fini_array_start))
        (*_(_fini_array_end)[--i])();
#ifdef __CPRIME_NATIVE_CRT__
#ifdef CPC_RUNTIME_DLL
#ifdef __CPRIME_UCRT__
    __cpc_execute_onexit_table();
#endif
    run_native_terminators();
#elif defined(__CPRIME_UCRT__)
    if (cpc_runtime_in_memory) __cpc_execute_onexit_table();
#endif
#endif /* __CPRIME_NATIVE_CRT__ */
}






