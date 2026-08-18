#include <windows.h>

int main()
{
    return ES_AWAYMODE_REQUIRED == 0x00000040 ? 0 : 1;
}
