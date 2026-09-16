#include <windows.h>
extern int native_static_value(void);
extern int native_construction_count(void);
extern int native_tls_increment(void);
static DWORD WINAPI worker(void *unused)
{
    int i;
    (void)unused;
    if (native_tls_increment() != 8 || native_tls_increment() != 9) return 1;
    for (i = 0; i < 1000; ++i)
        if (native_static_value() != 42) return 2;
    return 0;
}
int main(void)
{
    HANDLE threads[4];
    DWORD result;
    int i;
    if (native_tls_increment() != 8) return 1;
    for (i = 0; i < 4; ++i) {
        threads[i] = CreateThread(0, 0, worker, 0, 0, 0);
        if (!threads[i]) return 2;
    }
    for (i = 0; i < 4; ++i) {
        if (WaitForSingleObject(threads[i], INFINITE) != WAIT_OBJECT_0) return 3;
        if (!GetExitCodeThread(threads[i], &result) || result) return 4;
        CloseHandle(threads[i]);
    }
    return native_tls_increment() != 9 || native_construction_count() != 1;
}
