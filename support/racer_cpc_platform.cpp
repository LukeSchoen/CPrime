#define _BitScanForward64 cpc_windows_BitScanForward64
#include <windows.h>
#undef _BitScanForward64
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

extern "C"
{
void clClearAnyPreviousWindowsError()
{
  (void)GetLastError();
}

unsigned char _clAbsHelper____cpc_type_unsigned_char__false_Abs(
  const unsigned char *value)
{
  return *value;
}

long long _ftelli64(FILE *stream)
{
  return (long long)ftell(stream);
}

long _InterlockedExchangeAdd(volatile long *target, long value)
{
  long previous = *target;
  *target = previous + value;
  return previous;
}

unsigned char _BitScanForward64(unsigned long *index,
                                unsigned long long mask)
{
  unsigned long bit = 0;
  if (!mask)
    return 0;
  while (!(mask & 1))
  {
    mask >>= 1;
    ++bit;
  }
  *index = bit;
  return 1;
}

void __debugbreak()
{
}

size_t strnlen_s(const char *text, size_t maximum)
{
  size_t length = 0;
  if (!text)
    return 0;
  while (length < maximum && text[length])
    ++length;
  return length;
}

/* The PE backend currently emits a helper call for dynamic stack storage.
   Keep Racer linkable until that lowering uses the native stack probe. */
void *alloca(size_t size)
{
  return malloc(size);
}

double __cpc_ns_std_exp(double value)
{
  return exp(value);
}

void __cpc_ns_std_qsort(void *base, size_t count, size_t width,
                        int (*compare)(const void *, const void *))
{
  qsort(base, count, width, compare);
}

unsigned int __cpc_ns_std_thread_hardware_concurrency()
{
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors ? info.dwNumberOfProcessors : 1;
}

long long __cpc_ns_std_chrono_milliseconds(long long value)
{
  return value;
}

void __cpc_ns_std_this_thread_sleep_for(long long milliseconds)
{
  Sleep(milliseconds > 0 ? (DWORD)milliseconds : 0);
}
}
