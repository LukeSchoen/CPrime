#ifndef CPRIME_WINAPI_COMPAT_H
#define CPRIME_WINAPI_COMPAT_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
WINBASEAPI ULONGLONG WINAPI GetTickCount64(VOID);
#ifdef __cplusplus
}
#endif

#endif
