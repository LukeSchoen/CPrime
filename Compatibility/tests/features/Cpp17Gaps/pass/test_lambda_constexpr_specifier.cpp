// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: constexpr_lambda. A lambda may carry an explicit constexpr.
int main() {
  auto twice = [](int value) constexpr { return value * 2; };
  return twice(2) - 4;
}
