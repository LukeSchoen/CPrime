#ifndef _CPRIME_COMBASEAPI_H
#define _CPRIME_COMBASEAPI_H

#include <windows.h>

#ifndef WINOLEAPI
#define WINOLEAPI EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define WINOLEAPI_(type) EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#endif

typedef enum tagCOINITBASE {
    COINITBASE_MULTITHREADED = 0
} COINITBASE;

WINOLEAPI CoInitializeEx(LPVOID reserved, DWORD concurrency);
WINOLEAPI_(void) CoUninitialize(void);
WINOLEAPI_(LPVOID) CoTaskMemAlloc(SIZE_T size);
WINOLEAPI_(LPVOID) CoTaskMemRealloc(LPVOID memory, SIZE_T size);
WINOLEAPI_(void) CoTaskMemFree(LPVOID memory);

#endif
