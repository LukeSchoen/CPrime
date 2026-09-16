// EXPECT_COMPILE_ARGS: -lole32 -Werror
#include <combaseapi.h>
#include <shlobj.h>

int main()
{
    HRESULT (WINAPI *initialize)(LPVOID, DWORD) = &CoInitializeEx;
    void (WINAPI *uninitialize)(void) = &CoUninitialize;
    HRESULT first = initialize(0, COINITBASE_MULTITHREADED);
    if (first != S_OK) return 1;
    HRESULT second = initialize(0, COINITBASE_MULTITHREADED);
    if (second != S_FALSE) return 2;
    if (initialize(0, 2) != RPC_E_CHANGED_MODE) return 3;
    unsigned char *memory = (unsigned char *)CoTaskMemAlloc(4);
    if (!memory) return 4;
    memory[0] = 42;
    memory = (unsigned char *)CoTaskMemRealloc(memory, 16);
    if (!memory || memory[0] != 42) return 5;
    CoTaskMemFree(memory);
    uninitialize();
    uninitialize();
    if (initialize(0, 2) != S_OK) return 6;
    uninitialize();
    return 0;
}
