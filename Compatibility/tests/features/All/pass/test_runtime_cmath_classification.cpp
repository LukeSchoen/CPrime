#include <math.h>
#include <cmath>
int main() {
  union { unsigned int bits; float value; } small;
  small.bits = 1;
  double infinity = 1.0 / 0.0;
  double nan = 0.0 / 0.0;
  if (!::isinf(infinity) || !std::isinf(infinity)) return 1;
  if (!::isnan(nan) || !std::isnan(nan)) return 2;
  if (!std::signbit(-0.0) || std::signbit(0.0)) return 3;
  if (std::isnormal(small.value) || !std::isfinite(small.value)) return 4;
  if (!std::isnormal(1.0) || std::isfinite(infinity)) return 5;
  if (std::isnan(3) || std::isinf(3ULL) || std::isnormal(0)) return 6;
  if (!std::isfinite(3U) || !std::signbit(-3LL)) return 7;
  if (std::sqrt(9) != 3.0 || std::sqrt(16.0f) != 4.0f) return 8;
  if (std::sqrt(25.0L) != 5.0L) return 9;
  return 0;
}
