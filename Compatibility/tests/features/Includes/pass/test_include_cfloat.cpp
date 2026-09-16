#include <cfloat>

int main()
{
  if (FLT_RADIX != 2)
    return 1;
  if (FLT_MANT_DIG != 24 || DBL_MANT_DIG != 53)
    return 2;
  if (FLT_MAX <= FLT_MIN || DBL_MAX <= DBL_MIN)
    return 3;
  return 0;
}
