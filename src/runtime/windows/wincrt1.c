//+---------------------------------------------------------------------------

// _UNICODE for tchar.h, UNICODE for API
#include <tchar.h>

#include <windows.h>
#include <stdlib.h>
#include <cprime_crt.h>

#define __UNKNOWN_APP    0
#define __CONSOLE_APP    1
#define __GUI_APP        2
void __set_app_type(int);
void _controlfp(unsigned a, unsigned b);

#ifdef _UNICODE
#define __tgetmainargs __wgetmainargs
#define _twinstart _wwinstart
#define _runtwinmain _runwwinmain
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int);
#else
#define __tgetmainargs __getmainargs
#define _twinstart _winstart
#define _runtwinmain _runwinmain
#endif

typedef struct { int newmode; } _startupinfo;
int __cdecl __tgetmainargs(int *pargc, _TCHAR ***pargv, _TCHAR ***penv, int globb, _startupinfo*);

#include "crtinit.c"

static int go_winmain(int argc, _TCHAR **argv)
{
    STARTUPINFO si;
    _TCHAR *szCmd, *p;
    int fShow;
    int retval;
    _TCHAR *arg1 = argc > 1 ? argv[1] : NULL;

    GetStartupInfo(&si);
    if (si.dwFlags & STARTF_USESHOWWINDOW)
        fShow = si.wShowWindow;
    else
        fShow = SW_SHOWDEFAULT;

    szCmd = NULL, p = GetCommandLine();
    if (arg1)
        szCmd = _tcsstr(p, arg1);
    if (NULL == szCmd)
        szCmd = _tcsdup(__T(""));
    else if (szCmd > p && szCmd[-1] == __T('"'))
        --szCmd;
#if defined __i386__ || defined __x86_64__
    _controlfp(0x10000, 0x30000);
#endif
    run_ctors(argc, argv, _tenviron);
    retval = _tWinMain(GetModuleHandle(NULL), NULL, szCmd, fShow);
    run_dtors();
    return retval;
}

static LONG WINAPI catch_sig(EXCEPTION_POINTERS *ex)
{
  return _XcptFilter(ex->ExceptionRecord->ExceptionCode, ex);
}

int _twinstart(void)
{
    _startupinfo start_info_con = {0};
    SetUnhandledExceptionFilter(catch_sig);
    __set_app_type(__GUI_APP);
#ifdef __CPRIME_UCRT__
    initialize_arguments(0);
#else
    __tgetmainargs(&__argc, &__targv, &_tenviron, 0, &start_info_con);
#endif
    exit(go_winmain(__argc, __targv));
}

__attribute__((weak)) extern int __run_on_exit();

int _runtwinmain(int argc, /* as cpc passed in */ char **argv)
{
    int result;
    int program_argc = argc;
    _TCHAR **program_argv;
#ifdef __CPRIME_NATIVE_CRT__
    cpc_runtime_in_memory = 1;
#endif
#ifdef UNICODE
    _startupinfo start_info = {0};
#ifdef __CPRIME_UCRT__
    initialize_arguments(0);
#else
    __tgetmainargs(&__argc, &__targv, &_tenviron, 0, &start_info);
#endif
    /* may be wrong when cpc has received wildcards (*.c) */
    program_argc = __argc;
    program_argv = __targv;
    if (argc < program_argc) {
        program_argv += program_argc - argc;
        program_argc = argc;
    }
#else
    program_argv = argv;
#endif
    result = go_winmain(program_argc, program_argv);
    __run_on_exit(result);
    return result;
}






