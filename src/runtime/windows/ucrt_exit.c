/* Separate archive member: -run supplies its own atexit implementation so
   callbacks are drained before generated code is released. */
#ifdef __CPRIME_UCRT__
#include <stdlib.h>
int atexit(void (__cdecl *function)(void))
{
    return _onexit((_onexit_t)function) ? 0 : -1;
}
#endif
