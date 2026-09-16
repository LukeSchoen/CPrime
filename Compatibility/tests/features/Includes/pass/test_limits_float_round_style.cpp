// The floating-point style enumerations of <limits>; boost's numeric
// conversion selects a rounding policy with
// integral_c<std::float_round_style, std::round_to_nearest>.
#include <limits>

template<class T, T Value> struct integral_c { static const T value = Value; };

typedef integral_c<std::float_round_style, std::round_toward_zero> round2zero;

int main()
{
  if (round2zero::value != std::round_toward_zero) return 1;
  if (std::round_to_nearest == std::round_toward_zero) return 2;
  if (std::numeric_limits<float>::round_style != std::round_to_nearest) return 3;
  if (std::numeric_limits<int>::round_style != std::round_toward_zero) return 4;
  if (std::round_indeterminate >= 0) return 5;
  if (std::denorm_present != 1 || std::denorm_absent != 0) return 6;
  return 0;
}
