/* Small Windows/CRT compatibility entry points that are not exported by the
   legacy msvcrt import library used by the PE runtime.  Keep these in the
   owned runtime so generated programs never need project-specific shims. */

#include <stddef.h>
#include <stdio.h>
#include <stdatomic.h>
#include <io.h>

size_t strnlen_s(const char *text, size_t maximum)
{
    size_t length = 0;
    if (!text)
        return 0;
    while (length < maximum && text[length])
        ++length;
    return length;
}

long long _ftelli64(FILE *stream)
{
    return _telli64(_fileno(stream));
}

#include <windows.h>

unsigned int __cpc_windows_hardware_concurrency(void)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors ? info.dwNumberOfProcessors : 1;
}

void __cpc_windows_sleep_for_milliseconds(long long milliseconds)
{
    Sleep(milliseconds > 0 ? (DWORD)milliseconds : 0);
}
