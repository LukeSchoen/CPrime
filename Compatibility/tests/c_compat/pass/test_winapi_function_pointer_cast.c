// EXPECT_COMPILE_ONLY: 1
#include <windows.h>
typedef ULONGLONG (WINAPI *TickFn)(void);
void f(FARPROC p) {
    ULONGLONG (WINAPI *fn)(void);
    fn = (ULONGLONG (WINAPI *)(void))p;
}
void alias(FARPROC p) {
    TickFn fn;
    fn = (TickFn)p;
}
