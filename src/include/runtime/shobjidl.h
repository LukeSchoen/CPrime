#ifndef _CPC_SHOBJIDL_H
#define _CPC_SHOBJIDL_H
#include <shlobj.h>
#include <objidl.h>
#include <objbase.h>
#ifdef __cplusplus
typedef ULONG SFGAOF;
typedef DWORD SICHINTF;
struct IBindCtx;
typedef enum _SIGDN {
  SIGDN_NORMALDISPLAY=0, SIGDN_PARENTRELATIVEPARSING=(int)0x80018001,
  SIGDN_DESKTOPABSOLUTEPARSING=(int)0x80028000,
  SIGDN_PARENTRELATIVEEDITING=(int)0x80031001,
  SIGDN_DESKTOPABSOLUTEEDITING=(int)0x8004c000,
  SIGDN_FILESYSPATH=(int)0x80058000, SIGDN_URL=(int)0x80068000,
  SIGDN_PARENTRELATIVEFORADDRESSBAR=(int)0x8007c001,
  SIGDN_PARENTRELATIVE=(int)0x80080001, SIGDN_PARENTRELATIVEFORUI=(int)0x80094001
} SIGDN;
struct IShellItem : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE BindToHandler(IBindCtx*,REFGUID,REFIID,void**)=0;
  virtual HRESULT STDMETHODCALLTYPE GetParent(IShellItem**)=0;
  virtual HRESULT STDMETHODCALLTYPE GetDisplayName(SIGDN,LPWSTR*)=0;
  virtual HRESULT STDMETHODCALLTYPE GetAttributes(SFGAOF,SFGAOF*)=0;
  virtual HRESULT STDMETHODCALLTYPE Compare(IShellItem*,SICHINTF,int*)=0;
};
struct IModalWindow : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE Show(HWND)=0;
};
struct IFileDialogEvents;
struct IShellItemFilter;
typedef enum FDAP { FDAP_BOTTOM=0, FDAP_TOP=1 } FDAP;
typedef DWORD FILEOPENDIALOGOPTIONS;
enum _FILEOPENDIALOGOPTIONS {
  FOS_OVERWRITEPROMPT=2,FOS_STRICTFILETYPES=4,FOS_NOCHANGEDIR=8,
  FOS_PICKFOLDERS=0x20,FOS_FORCEFILESYSTEM=0x40,FOS_ALLNONSTORAGEITEMS=0x80,
  FOS_NOVALIDATE=0x100,FOS_ALLOWMULTISELECT=0x200,FOS_PATHMUSTEXIST=0x800,
  FOS_FILEMUSTEXIST=0x1000,FOS_CREATEPROMPT=0x2000,FOS_SHAREAWARE=0x4000,
  FOS_NOREADONLYRETURN=0x8000,FOS_NOTESTFILECREATE=0x10000,
  FOS_HIDEMRUPLACES=0x20000,FOS_HIDEPINNEDPLACES=0x40000,
  FOS_NODEREFERENCELINKS=0x100000,FOS_OKBUTTONNEEDSINTERACTION=0x200000,
  FOS_DONTADDTORECENT=0x2000000,FOS_FORCESHOWHIDDEN=0x10000000,
  FOS_DEFAULTNOMINIMODE=0x20000000,FOS_FORCEPREVIEWPANEON=0x40000000,
  FOS_SUPPORTSTREAMABLEITEMS=0x80000000u
};
struct IFileDialog : IModalWindow {
  virtual HRESULT STDMETHODCALLTYPE SetFileTypes(UINT,const COMDLG_FILTERSPEC*)=0;
  virtual HRESULT STDMETHODCALLTYPE SetFileTypeIndex(UINT)=0;
  virtual HRESULT STDMETHODCALLTYPE GetFileTypeIndex(UINT*)=0;
  virtual HRESULT STDMETHODCALLTYPE Advise(IFileDialogEvents*,DWORD*)=0;
  virtual HRESULT STDMETHODCALLTYPE Unadvise(DWORD)=0;
  virtual HRESULT STDMETHODCALLTYPE SetOptions(FILEOPENDIALOGOPTIONS)=0;
  virtual HRESULT STDMETHODCALLTYPE GetOptions(FILEOPENDIALOGOPTIONS*)=0;
  virtual HRESULT STDMETHODCALLTYPE SetDefaultFolder(IShellItem*)=0;
  virtual HRESULT STDMETHODCALLTYPE SetFolder(IShellItem*)=0;
  virtual HRESULT STDMETHODCALLTYPE GetFolder(IShellItem**)=0;
  virtual HRESULT STDMETHODCALLTYPE GetCurrentSelection(IShellItem**)=0;
  virtual HRESULT STDMETHODCALLTYPE SetFileName(LPCWSTR)=0;
  virtual HRESULT STDMETHODCALLTYPE GetFileName(LPWSTR*)=0;
  virtual HRESULT STDMETHODCALLTYPE SetTitle(LPCWSTR)=0;
  virtual HRESULT STDMETHODCALLTYPE SetOkButtonLabel(LPCWSTR)=0;
  virtual HRESULT STDMETHODCALLTYPE SetFileNameLabel(LPCWSTR)=0;
  virtual HRESULT STDMETHODCALLTYPE GetResult(IShellItem**)=0;
  virtual HRESULT STDMETHODCALLTYPE AddPlace(IShellItem*,FDAP)=0;
  virtual HRESULT STDMETHODCALLTYPE SetDefaultExtension(LPCWSTR)=0;
  virtual HRESULT STDMETHODCALLTYPE Close(HRESULT)=0;
  virtual HRESULT STDMETHODCALLTYPE SetClientGuid(REFGUID)=0;
  virtual HRESULT STDMETHODCALLTYPE ClearClientData()=0;
  virtual HRESULT STDMETHODCALLTYPE SetFilter(IShellItemFilter*)=0;
};
DEFINE_GUID(IID_IFileDialog,0x42f85136,0xdb7e,0x439c,0x85,0xf1,0xe4,0x07,0x5d,0x13,0x5f,0xc8);
DEFINE_GUID(IID_IShellItem,0x43826d1e,0xe718,0x42ee,0xbc,0x55,0xa1,0xe2,0x61,0xc3,0x7b,0xfe);
DEFINE_GUID(CLSID_FileOpenDialog,0xdc1c5a9c,0xe88a,0x4dde,0xa5,0xa1,0x60,0xf8,0x2a,0x20,0xae,0xf7);
template<class T> const IID& __cpc_interface_iid(T**);
template<> inline const IID& __cpc_interface_iid(IFileDialog**) {return IID_IFileDialog;}
template<> inline const IID& __cpc_interface_iid(IShellItem**) {return IID_IShellItem;}
#ifndef IID_PPV_ARGS
#define IID_PPV_ARGS(pp) __cpc_interface_iid(pp), reinterpret_cast<void**>(pp)
#endif
struct IShellFolder;
#define SFGAO_FOLDER 0x20000000
SHSTDAPI SHParseDisplayName(PCWSTR,IBindCtx*,PIDLIST_ABSOLUTE*,SFGAOF,SFGAOF*);
SHSTDAPI SHCreateShellItem(PCIDLIST_ABSOLUTE,IShellFolder*,PCUITEMID_CHILD,IShellItem**);
struct IShellLinkA : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetPath(
        LPSTR pszFile,
        int cch,
        WIN32_FIND_DATAA *pfd,
        DWORD fFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetIDList(
        PIDLIST_ABSOLUTE *ppidl) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetIDList(
        PCIDLIST_ABSOLUTE pidl) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDescription(
        LPSTR pszName,
        int cch) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDescription(
        LPCSTR pszName) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetWorkingDirectory(
        LPSTR pszDir,
        int cch) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetWorkingDirectory(
        LPCSTR pszDir) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetArguments(
        LPSTR pszArgs,
        int cch) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetArguments(
        LPCSTR pszArgs) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetHotkey(
        WORD *pwHotkey) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetHotkey(
        WORD wHotkey) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetShowCmd(
        int *piShowCmd) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetShowCmd(
        int iShowCmd) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetIconLocation(
        LPSTR pszIconPath,
        int cch,
        int *piIcon) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetIconLocation(
        LPCSTR pszIconPath,
        int iIcon) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetRelativePath(
        LPCSTR pszPathRel,
        DWORD dwReserved) = 0;

    virtual HRESULT STDMETHODCALLTYPE Resolve(
        HWND hwnd,
        DWORD fFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetPath(
        LPCSTR pszFile) = 0;

};
struct IShellLinkW : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetPath(
        LPWSTR pszFile,
        int cch,
        WIN32_FIND_DATAW *pfd,
        DWORD fFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetIDList(
        PIDLIST_ABSOLUTE *ppidl) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetIDList(
        PCIDLIST_ABSOLUTE pidl) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDescription(
        LPWSTR pszName,
        int cch) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDescription(
        LPCWSTR pszName) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetWorkingDirectory(
        LPWSTR pszDir,
        int cch) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetWorkingDirectory(
        LPCWSTR pszDir) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetArguments(
        LPWSTR pszArgs,
        int cch) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetArguments(
        LPCWSTR pszArgs) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetHotkey(
        WORD *pwHotkey) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetHotkey(
        WORD wHotkey) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetShowCmd(
        int *piShowCmd) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetShowCmd(
        int iShowCmd) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetIconLocation(
        LPWSTR pszIconPath,
        int cch,
        int *piIcon) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetIconLocation(
        LPCWSTR pszIconPath,
        int iIcon) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetRelativePath(
        LPCWSTR pszPathRel,
        DWORD dwReserved) = 0;

    virtual HRESULT STDMETHODCALLTYPE Resolve(
        HWND hwnd,
        DWORD fFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetPath(
        LPCWSTR pszFile) = 0;

};
#endif
DEFINE_GUID(IID_IShellLinkA, 0x000214ee, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IShellLinkW, 0x000214f9, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_ShellLink, 0x00021401, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);
#ifdef UNICODE
#define IShellLink IShellLinkW
#define IID_IShellLink IID_IShellLinkW
#else
#define IShellLink IShellLinkA
#define IID_IShellLink IID_IShellLinkA
#endif
#endif
