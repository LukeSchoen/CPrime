#ifndef CPC_WINDOWS_CRT_STARTUP_H
#define CPC_WINDOWS_CRT_STARTUP_H

#ifdef __CPRIME_UCRT__
int __cdecl _configure_narrow_argv(int);
int __cdecl _configure_wide_argv(int);
int __cdecl _initialize_narrow_environment(void);
int __cdecl _initialize_wide_environment(void);
void __cdecl _set_app_type(int);
int __cdecl _seh_filter_exe(unsigned long, struct _EXCEPTION_POINTERS *);
#define __set_app_type _set_app_type
#define _XcptFilter _seh_filter_exe
#endif

#endif
