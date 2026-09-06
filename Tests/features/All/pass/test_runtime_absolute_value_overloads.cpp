#include <cstdlib>
#include <cmath>
#include <type_traits>
#include <stdint.h>
#include <string.h>

static_assert(std::is_same<decltype(std::abs(-1)), int>::value, "int overload");
static_assert(std::is_same<decltype(std::abs(-1L)), long>::value, "long overload");
static_assert(std::is_same<decltype(std::abs(-1LL)), long long>::value, "long long overload");
static_assert(std::is_same<decltype(std::abs(-0.5f)), float>::value, "float overload");
static_assert(std::is_same<decltype(std::abs(-0.5)), double>::value, "double overload");
static_assert(std::is_same<decltype(std::abs(-0.5L)), long double>::value, "long double overload");
static_assert(std::is_same<decltype(std::fabs(-0.5f)), float>::value, "fabs float overload");
static_assert(std::is_same<decltype(std::fabs(-0.5L)), long double>::value, "fabs long double overload");
static_assert(std::is_same<decltype(std::fabs(-3)), double>::value, "fabs integral overload");
static_assert(std::is_integral<signed char>::value && std::is_integral<unsigned char>::value, "byte integers");
static_assert(std::is_integral<short>::value && std::is_integral<unsigned short>::value, "short integers");
static_assert(std::is_integral<int>::value && std::is_integral<unsigned int>::value, "int integers");
static_assert(std::is_integral<long>::value && std::is_integral<unsigned long>::value, "long integers");
static_assert(std::is_integral<long long>::value && std::is_integral<unsigned long long>::value, "long long integers");
static_assert(std::is_integral<wchar_t>::value && std::is_integral<char16_t>::value && std::is_integral<char32_t>::value, "wide characters");
static_assert(std::is_integral<const int>::value && std::is_integral<volatile unsigned>::value && std::is_integral<const volatile long long>::value, "cv integers");
static_assert(!std::is_integral<int&>::value && !std::is_integral<double>::value, "non integers");
static_assert(std::is_floating_point<const float>::value && std::is_floating_point<volatile double>::value && std::is_floating_point<const volatile long double>::value, "cv floating types");
static_assert(!std::is_floating_point<double&>::value && !std::is_floating_point<int>::value, "non floating types");

static int check_float(uint32_t bits)
{
  float input;
  memcpy(&input, &bits, sizeof(input));
  volatile float value = input;
  float results[] = { ::fabsf(value), ::abs(value), std::fabs(value), std::abs(value) };
  for (unsigned i = 0; i < sizeof(results) / sizeof(results[0]); ++i)
  {
    uint32_t actual;
    memcpy(&actual, &results[i], sizeof(actual));
    if (actual != (bits & UINT32_C(0x7fffffff))) return 1;
  }
  return 0;
}

static int check_double(uint64_t bits)
{
  double input;
  memcpy(&input, &bits, sizeof(input));
  volatile double value = input;
  double results[] = { ::fabs(value), ::abs(value), std::fabs(value), std::abs(value) };
  for (unsigned i = 0; i < sizeof(results) / sizeof(results[0]); ++i)
  {
    uint64_t actual;
    memcpy(&actual, &results[i], sizeof(actual));
    if (actual != (bits & UINT64_C(0x7fffffffffffffff))) return 1;
  }
  return 0;
}

int main()
{
  if (std::abs(-3L) != 3L || std::abs(-5000000000LL) != 5000000000LL) return 1;
  if (std::abs(-0.5f) != 0.5f || std::abs(-0.25) != 0.25 || std::abs(-0.125L) != 0.125L) return 2;
  if (std::signbit(std::abs(-0.0f)) || std::signbit(std::abs(-0.0))) return 3;
  const uint32_t floatBits[] = { 0, UINT32_C(0x80000000), UINT32_C(0x80000001), UINT32_C(0x7f800000), UINT32_C(0xff800000), UINT32_C(0xffc12345) };
  for (unsigned i = 0; i < sizeof(floatBits) / sizeof(floatBits[0]); ++i)
    if (check_float(floatBits[i])) return 4;
  const uint64_t doubleBits[] = { 0, UINT64_C(0x8000000000000000), UINT64_C(0x8000000000000001), UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000), UINT64_C(0xfff8123456789abc) };
  for (unsigned i = 0; i < sizeof(doubleBits) / sizeof(doubleBits[0]); ++i)
    if (check_double(doubleBits[i])) return 5;
  volatile long double negativeZero = -0.0L;
  volatile long double negativeInfinity = -HUGE_VALL;
  if (std::signbit(::fabsl(negativeZero)) || std::signbit(::abs(negativeZero)) || std::signbit(std::abs(negativeZero))) return 6;
  if (::fabsl(negativeInfinity) != HUGE_VALL || std::abs(negativeInfinity) != HUGE_VALL) return 7;
  if (std::fabs(-3) != 3.0 || std::fabs(-5000000000LL) != 5000000000.0) return 8;
  if (std::signbit(std::fabs(negativeZero)) || std::fabs(negativeInfinity) != HUGE_VALL) return 9;
  return 0;
}
