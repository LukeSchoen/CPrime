/* Boost.ContainerHash's float hashing compiles
   `numeric_limits<Float>::is_iec559 && ... digits ... radix ...
   max_exponent`, so the floating and integer specializations carry the
   standard member set and the values of the IEEE-754 formats CPC targets. */
#include <limits>

static_assert(std::numeric_limits<float>::is_iec559, "float iec559");
static_assert(std::numeric_limits<double>::is_iec559, "double iec559");
static_assert(std::numeric_limits<float>::digits == 24, "float digits");
static_assert(std::numeric_limits<double>::digits == 53, "double digits");
static_assert(std::numeric_limits<float>::radix == 2, "float radix");
static_assert(std::numeric_limits<float>::max_exponent == 128, "float exponent");
static_assert(std::numeric_limits<double>::max_exponent == 1024, "double exponent");
static_assert(std::numeric_limits<float>::min_exponent == -125, "float min exponent");
static_assert(std::numeric_limits<float>::digits10 == 6, "float digits10");
static_assert(std::numeric_limits<double>::digits10 == 15, "double digits10");
static_assert(std::numeric_limits<float>::max_digits10 == 9, "float max digits10");
static_assert(std::numeric_limits<double>::max_digits10 == 17, "double max digits10");
static_assert(std::numeric_limits<float>::has_infinity, "float infinity");
static_assert(std::numeric_limits<int>::is_exact, "int exact");
static_assert(std::numeric_limits<int>::is_modulo == false, "signed not modulo");
static_assert(std::numeric_limits<unsigned int>::is_modulo, "unsigned modulo");
static_assert(std::numeric_limits<int>::digits10 == 9, "int digits10");
static_assert(!std::numeric_limits<int>::is_iec559, "int not iec559");

int main()
{
  if (!(std::numeric_limits<float>::denorm_min() > 0)) return 1;
  if (!(std::numeric_limits<float>::denorm_min() < std::numeric_limits<float>::min())) return 2;
  if (std::numeric_limits<int>::denorm_min() != 0) return 3;
  if (std::numeric_limits<float>::round_error() != 0.5f) return 4;
  if (std::numeric_limits<int>::round_error() != 0) return 5;
  if (!std::numeric_limits<float>::has_quiet_NaN) return 6;
  return 0;
}
