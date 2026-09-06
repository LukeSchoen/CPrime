// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

int narrow_v(char (&buffer)[64], const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int result = vsprintf_s(buffer, format, args);
  va_end(args);
  return result;
}

int wide_v(wchar_t (&buffer)[64], const wchar_t *format, ...)
{
  va_list args;
  va_start(args, format);
  int result = vswprintf_s(buffer, format, args);
  va_end(args);
  return result;
}

int main()
{
  char buffer[64];
  wchar_t wide[64];
  if (sprintf_s(buffer, "%s %d %.1f %llx", "value", 7, 2.5, 0x123456789LL) != 21
      || strcmp(buffer, "value 7 2.5 123456789")) return 1;
  if (narrow_v(buffer, "%s %d", "next", 12) != 7 || strcmp(buffer, "next 12")) return 2;
  if (swprintf_s(wide, L"%ls %d", L"wide", 23) != 7 || wcscmp(wide, L"wide 23")) return 3;
  if (wide_v(wide, L"%ls %d", L"last", 34) != 7 || wcscmp(wide, L"last 34")) return 4;
  if (sprintf_s(buffer, sizeof(buffer), "%s", "explicit") != 8 || strcmp(buffer, "explicit")) return 5;
  return 0;
}
