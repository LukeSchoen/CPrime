#include <windows.h>
static volatile LONG ready;
static HANDLE proceed;
static DWORD WINAPI worker(void *argument) {
    int value = (int)(long long)argument;
    try {
        try { throw value; }
        catch (int code) {
            if (code != value) return 1;
            InterlockedIncrement(&ready);
            if (WaitForSingleObject(proceed, 5000) != WAIT_OBJECT_0) return 2;
            throw;
        }
    } catch (int code) { return code == value ? 0 : 3; }
    return 4;
}
int main() {
    HANDLE threads[2];
    DWORD first, second;
    proceed = CreateEventA(0, TRUE, FALSE, 0);
    if (!proceed) return 1;
    threads[0] = CreateThread(0, 0, worker, (void*)17, 0, 0);
    threads[1] = CreateThread(0, 0, worker, (void*)29, 0, 0);
    if (!threads[0] || !threads[1]) return 2;
    for (int index = 0; ready != 2 && index < 5000; ++index) Sleep(1);
    SetEvent(proceed);
    if (WaitForMultipleObjects(2, threads, TRUE, 5000) != WAIT_OBJECT_0) return 3;
    GetExitCodeThread(threads[0], &first);
    GetExitCodeThread(threads[1], &second);
    CloseHandle(threads[0]); CloseHandle(threads[1]); CloseHandle(proceed);
    return first || second || ready != 2;
}
