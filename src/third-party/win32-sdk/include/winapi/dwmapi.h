#ifndef _DWMAPI_H_
#define _DWMAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI DwmSetWindowAttribute(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);

#ifdef __cplusplus
}
#endif

#endif
