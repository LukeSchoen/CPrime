#ifndef _CPC_OBJIDL_H
#define _CPC_OBJIDL_H
#include <unknwn.h>
#ifndef _OLECHAR_DEFINED
#define _OLECHAR_DEFINED
typedef WCHAR OLECHAR;
typedef OLECHAR *LPOLESTR;
typedef const OLECHAR *LPCOLESTR;
#endif
#define STGM_READ 0x00000000L
#define STGM_WRITE 0x00000001L
#define STGM_READWRITE 0x00000002L
#ifdef __cplusplus
struct IPersist : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetClassID(
        CLSID *pClassID) = 0;

};
struct IPersistFile : public IPersist
{
    virtual HRESULT STDMETHODCALLTYPE IsDirty(
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Load(
        LPCOLESTR pszFileName,
        DWORD dwMode) = 0;

    virtual HRESULT STDMETHODCALLTYPE Save(
        LPCOLESTR pszFileName,
        WINBOOL fRemember) = 0;

    virtual HRESULT STDMETHODCALLTYPE SaveCompleted(
        LPCOLESTR pszFileName) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetCurFile(
        LPOLESTR *ppszFileName) = 0;

};
#endif
DEFINE_GUID(IID_IPersist, 0x0000010c, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IPersistFile, 0x0000010b, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
#endif
