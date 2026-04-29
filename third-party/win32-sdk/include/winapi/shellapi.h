/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_SHELLAPI
#define _INC_SHELLAPI

#ifndef WINSHELLAPI
#if !defined(_SHELL32_)
#define WINSHELLAPI DECLSPEC_IMPORT
#else
#define WINSHELLAPI
#endif
#endif

#ifndef SHSTDAPI
#if !defined(_SHELL32_)
#define SHSTDAPI EXTERN_C DECLSPEC_IMPORT HRESULT WINAPI
#define SHSTDAPI_(type) EXTERN_C DECLSPEC_IMPORT type WINAPI
#else
#define SHSTDAPI STDAPI
#define SHSTDAPI_(type) STDAPI_(type)
#endif
#endif

/* SHDOCAPI[_] definitions not required in this Cplus minimal header */

#if !defined(_WIN64)
#include <pshpack1.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef UNICODE
#define ShellExecute ShellExecuteW
#define ShellExecuteEx ShellExecuteExW
#define FindExecutable FindExecutableW
#define DragQueryFile DragQueryFileW
#else
#define ShellExecute ShellExecuteA
#define ShellExecuteEx ShellExecuteExA
#define FindExecutable FindExecutableA
#define DragQueryFile DragQueryFileA
#endif

  /* minimal subset distributed with Cplus. nShowCmd is at winuser.h */
  typedef HANDLE HDROP;
  typedef struct _SHELLEXECUTEINFOA {
    DWORD cbSize;
    ULONG fMask;
    HWND hwnd;
    LPCSTR lpVerb;
    LPCSTR lpFile;
    LPCSTR lpParameters;
    LPCSTR lpDirectory;
    int nShow;
    HINSTANCE hInstApp;
    LPVOID lpIDList;
    LPCSTR lpClass;
    HKEY hkeyClass;
    DWORD dwHotKey;
    HANDLE hIcon;
    HANDLE hProcess;
  } SHELLEXECUTEINFOA, *LPSHELLEXECUTEINFOA;

  typedef struct _SHELLEXECUTEINFOW {
    DWORD cbSize;
    ULONG fMask;
    HWND hwnd;
    LPCWSTR lpVerb;
    LPCWSTR lpFile;
    LPCWSTR lpParameters;
    LPCWSTR lpDirectory;
    int nShow;
    HINSTANCE hInstApp;
    LPVOID lpIDList;
    LPCWSTR lpClass;
    HKEY hkeyClass;
    DWORD dwHotKey;
    HANDLE hIcon;
    HANDLE hProcess;
  } SHELLEXECUTEINFOW, *LPSHELLEXECUTEINFOW;

#define SEE_MASK_DEFAULT 0x00000000
#define SEE_MASK_CLASSNAME 0x00000001
#define SEE_MASK_CLASSKEY 0x00000003
#define SEE_MASK_IDLIST 0x00000004
#define SEE_MASK_INVOKEIDLIST 0x0000000c
#define SEE_MASK_HOTKEY 0x00000020
#define SEE_MASK_NOCLOSEPROCESS 0x00000040
#define SEE_MASK_CONNECTNETDRV 0x00000080
#define SEE_MASK_FLAG_NO_UI 0x00000400
#define SEE_MASK_UNICODE 0x00004000
#define SEE_MASK_NO_CONSOLE 0x00008000
#define SEE_MASK_ASYNCOK 0x00100000
#define SEE_MASK_NOASYNC 0x00000100
#define SEE_MASK_WAITFORINPUTIDLE 0x02000000
#define SEE_MASK_FLAG_LOG_USAGE 0x04000000

  SHSTDAPI_(HINSTANCE) ShellExecuteA(HWND hwnd,LPCSTR lpOperation,LPCSTR lpFile,LPCSTR lpParameters,LPCSTR lpDirectory,INT nShowCmd);
  SHSTDAPI_(HINSTANCE) ShellExecuteW(HWND hwnd,LPCWSTR lpOperation,LPCWSTR lpFile,LPCWSTR lpParameters,LPCWSTR lpDirectory,INT nShowCmd);
  SHSTDAPI_(WINBOOL) ShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo);
  SHSTDAPI_(WINBOOL) ShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo);
  SHSTDAPI_(HINSTANCE) FindExecutableA(LPCSTR lpFile,LPCSTR lpDirectory,LPSTR lpResult);
  SHSTDAPI_(HINSTANCE) FindExecutableW(LPCWSTR lpFile,LPCWSTR lpDirectory,LPWSTR lpResult);
  SHSTDAPI_(LPWSTR *) CommandLineToArgvW(LPCWSTR lpCmdLine,int*pNumArgs);
  void WINAPI DragAcceptFiles(HWND hWnd, WINBOOL fAccept);
  UINT WINAPI DragQueryFileA(HDROP hDrop, UINT iFile, LPSTR lpszFile, UINT cch);
  UINT WINAPI DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch);
  void WINAPI DragFinish(HDROP hDrop);

#ifdef __cplusplus
}
#endif

#if !defined(_WIN64)
#include <poppack.h>
#endif
#endif
