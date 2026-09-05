#include <windows.h>
#include <cprime_exception.h>
static int destroyed;
static void *retained;
struct Error {
    int value;
    Error(int value) : value(value) {}
    ~Error() { ++destroyed; }
};
static DWORD WINAPI worker(void *) {
    try { throw Error(163); }
    catch (...) { retained = __cpc_eh_current_exception(); }
    return 0;
}
int main() {
    if (__cpc_eh_current_exception()) return 1;
    HANDLE thread = CreateThread(0, 0, worker, 0, 0, 0);
    if (!thread || WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0) return 2;
    CloseHandle(thread);
    if (!retained || destroyed) return 3;
    __cpc_eh_retain_exception(retained);
    try { __cpc_eh_rethrow_exception(retained); }
    catch (const Error& error) {
        if (error.value != 163 || destroyed || __cpc_eh_uncaught_exceptions()) return 4;
    }
    __cpc_eh_release_exception(retained);
    if (destroyed) return 5;
    __cpc_eh_release_exception(retained);
    return destroyed != 1;
}
