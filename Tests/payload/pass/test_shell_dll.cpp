// EXPECT_COMPILE_ARGS: -lole32 -DUNICODE
#include <initguid.h>
#include <winapi/shobjidl.h>
#include <shobjidl.h>
#include <objbase.h>
#include <objidl.h>
int main() {
    IShellLinkW *link = 0;
    IPersistFile *persist = 0;
    if (FAILED(CoInitializeEx(0, COINIT_APARTMENTTHREADED))) return 1;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, 0, CLSCTX_INPROC_SERVER,
                                     IID_IShellLinkW, (void**)&link);
    if (FAILED(result) || !link) return 2;
    if (FAILED(link->SetDescription(L"portable shell ABI"))) return 3;
    wchar_t description[64];
    if (FAILED(link->GetDescription(description, 64))) return 4;
    if (lstrcmpW(description, L"portable shell ABI")) return 5;
    if (FAILED(link->QueryInterface(IID_IPersistFile, (void**)&persist))) return 6;
    persist->Release();
    link->Release();
    CoUninitialize();
    return 0;
}
