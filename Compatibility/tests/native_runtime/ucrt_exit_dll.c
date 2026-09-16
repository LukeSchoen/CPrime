#include <stdlib.h>
extern void native_register_exit(void (*callback)(void));
static int *state;
static void first(void) { *state = *state * 10 + 1; }
static void second(void) { *state = *state * 10 + 2; }
static int third(void) { *state = *state * 10 + 3; return 0; }
__declspec(dllexport) void register_module_callbacks(int *result)
{
    state = result;
    if (atexit(first)) abort();
    native_register_exit(second);
    if (!_onexit(third)) abort();
}
