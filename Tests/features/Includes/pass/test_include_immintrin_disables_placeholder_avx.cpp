#define MA_SUPPORT_AVX2
#include <immintrin.h>

#ifdef MA_SUPPORT_AVX2
#error MA_SUPPORT_AVX2 should be disabled by the CPC placeholder immintrin.h
#endif

int main()
{
  return 0;
}
