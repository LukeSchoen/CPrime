#include <windows.h>
extern "C" int native_tls_check(int, int);
extern "C" int native_init_order;
extern "C" int native_process_order;
extern "C" int native_tls_destructor_count;
static DWORD WINAPI worker(void *) {
    return native_tls_check(317, 29) || native_tls_check(29, 31);
}
int main() {
    if (native_process_order != 12 || native_init_order != 123) return 1;
    if (native_tls_check(317, 23)) return 2;
    HANDLE thread = CreateThread(0, 0, worker, 0, 0, 0);
    if (!thread) return 3;
    if (WaitForSingleObject(thread, 10000) != WAIT_OBJECT_0) return 4;
    DWORD result = 99;
    if (!GetExitCodeThread(thread, &result) || result) return 5;
    CloseHandle(thread);
    if (native_tls_destructor_count != 1) return 7;
    return native_tls_check(23, 37) ? 6 : 0;
}
