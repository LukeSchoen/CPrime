// EXPECT_COMPILE_ARGS: -std=c++17
// The unpacked make_tuple/tie/apply spellings stop at six parameters, so an
// arity above six has no definition to link against.
#include <tuple>

static int plus7(int a, int b, int c, int d, int e, int f, int g) {
  return a + b + c + d + e + f + g;
}

int main() {
  std::tuple<int, int, int, int, int, int, int> values =
      std::make_tuple(1, 2, 3, 4, 5, 6, 7);
  if (std::get<6>(values) != 7) return 1;

  return std::apply(plus7, values) - 28;
}
