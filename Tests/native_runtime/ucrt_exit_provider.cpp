#include <stdlib.h>
extern "C" void native_register_exit(void (*callback)(void))
{
    if (atexit(callback)) abort();
}
