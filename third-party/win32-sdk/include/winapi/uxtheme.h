#ifndef _UXTHEME_H_
#define _UXTHEME_H_

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);

#ifdef __cplusplus
}
#endif

#endif
