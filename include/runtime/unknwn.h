#ifndef _CPC_UNKNWN_H
#define _CPC_UNKNWN_H
#include <windows.h>
#include <basetyps.h>
#ifndef interface
#define interface struct
#endif
#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#define END_INTERFACE
#endif
#include <guiddef.h>
#ifdef __cplusplus
struct IUnknown {
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) = 0;
  virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
  virtual ULONG STDMETHODCALLTYPE Release() = 0;
};
#else
typedef struct IUnknown IUnknown;
typedef struct IUnknownVtbl {
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(IUnknown*,REFIID,void**);
  ULONG (STDMETHODCALLTYPE *AddRef)(IUnknown*);
  ULONG (STDMETHODCALLTYPE *Release)(IUnknown*);
} IUnknownVtbl;
struct IUnknown { const IUnknownVtbl* lpVtbl; };
#endif
typedef IUnknown* LPUNKNOWN;
DEFINE_GUID(IID_IUnknown,0,0,0,0xc0,0,0,0,0,0,0,0x46);
#endif
