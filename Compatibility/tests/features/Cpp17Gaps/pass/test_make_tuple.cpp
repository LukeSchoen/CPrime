// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_apply_tuple. std::make_tuple is declared but never defined.
// The tuple constructor gap in test_tuple_construction_and_get.cpp has to close
// before this test reaches that undefined symbol.
#include <tuple>

int main() {
  std::tuple<int, int> arguments = std::make_tuple(1, 2);
  if (std::get<0>(arguments) != 1) return 1;
  return std::get<1>(arguments) - 2;
}
