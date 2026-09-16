#ifndef _COMMDLG_H_
#define _COMMDLG_H_
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagOFNW {
  DWORD        lStructSize;
  HWND         hwndOwner;
  HINSTANCE    hInstance;
  LPCWSTR      lpstrFilter;
  LPWSTR       lpstrCustomFilter;
  DWORD        nMaxCustFilter;
  DWORD        nFilterIndex;
  LPWSTR       lpstrFile;
  DWORD        nMaxFile;
  LPWSTR       lpstrFileTitle;
  DWORD        nMaxFileTitle;
  LPCWSTR      lpstrInitialDir;
  LPCWSTR      lpstrTitle;
  DWORD        Flags;
  WORD         nFileOffset;
  WORD         nFileExtension;
  LPCWSTR      lpstrDefExt;
  LPARAM       lCustData;
  void        *lpfnHook;
  LPCWSTR      lpTemplateName;
  void        *pvReserved;
  DWORD        dwReserved;
  DWORD        FlagsEx;
} OPENFILENAMEW, *LPOPENFILENAMEW;

typedef struct tagOFNA {
  DWORD lStructSize;
  HWND hwndOwner;
  HINSTANCE hInstance;
  LPCSTR lpstrFilter;
  LPSTR lpstrCustomFilter;
  DWORD nMaxCustFilter;
  DWORD nFilterIndex;
  LPSTR lpstrFile;
  DWORD nMaxFile;
  LPSTR lpstrFileTitle;
  DWORD nMaxFileTitle;
  LPCSTR lpstrInitialDir;
  LPCSTR lpstrTitle;
  DWORD Flags;
  WORD nFileOffset;
  WORD nFileExtension;
  LPCSTR lpstrDefExt;
  LPARAM lCustData;
  void *lpfnHook;
  LPCSTR lpTemplateName;
  void *pvReserved;
  DWORD dwReserved;
  DWORD FlagsEx;
} OPENFILENAMEA, *LPOPENFILENAMEA;

#define OFN_HIDEREADONLY      0x00000004
#define OFN_OVERWRITEPROMPT   0x00000002
#define OFN_FILEMUSTEXIST     0x00001000
#define OFN_NOCHANGEDIR       0x00000008
#define OFN_EXPLORER          0x00080000

WINBOOL WINAPI GetOpenFileNameW(LPOPENFILENAMEW);
WINBOOL WINAPI GetSaveFileNameW(LPOPENFILENAMEW);
WINBOOL WINAPI GetOpenFileNameA(LPOPENFILENAMEA);
WINBOOL WINAPI GetSaveFileNameA(LPOPENFILENAMEA);
#ifdef UNICODE
typedef OPENFILENAMEW OPENFILENAME;
typedef LPOPENFILENAMEW LPOPENFILENAME;
#define GetOpenFileName GetOpenFileNameW
#define GetSaveFileName GetSaveFileNameW
#else
typedef OPENFILENAMEA OPENFILENAME;
typedef LPOPENFILENAMEA LPOPENFILENAME;
#define GetOpenFileName GetOpenFileNameA
#define GetSaveFileName GetSaveFileNameA
#endif

#ifdef __cplusplus
}
#endif

#endif
