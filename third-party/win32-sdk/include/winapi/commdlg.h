#ifndef _COMMDLG_H_
#define _COMMDLG_H_

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

#define OFN_HIDEREADONLY      0x00000004
#define OFN_OVERWRITEPROMPT   0x00000002
#define OFN_FILEMUSTEXIST     0x00001000

WINBOOL WINAPI GetOpenFileNameW(LPOPENFILENAMEW);
WINBOOL WINAPI GetSaveFileNameW(LPOPENFILENAMEW);

#ifdef __cplusplus
}
#endif

#endif
