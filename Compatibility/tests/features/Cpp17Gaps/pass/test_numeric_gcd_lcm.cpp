// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: numeric_gcd_lcm. <numeric> provides std::gcd and std::lcm,
// including a negative operand, with no retained case before this one.

#include <numeric>

int main()
{
  if (std::gcd(12, 18) != 6) return 1;
  if (std::gcd(-12, 18) != 6) return 2;
  if (std::lcm(4, 6) != 12) return 3;
  return 0;
}
