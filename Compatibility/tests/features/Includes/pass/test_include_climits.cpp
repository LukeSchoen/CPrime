#include <climits>

#if CHAR_BIT != 8
#error CPC currently targets 8-bit bytes
#endif

#if INT_MAX != 2147483647
#error CPC currently targets 32-bit int
#endif

int main()
{
  if (SCHAR_MIN != -128 || SCHAR_MAX != 127)
    return 1;
  if (UINT_MAX != 0xffffffffU)
    return 2;
  if (LLONG_MAX != 9223372036854775807LL)
    return 3;
  return 0;
}
