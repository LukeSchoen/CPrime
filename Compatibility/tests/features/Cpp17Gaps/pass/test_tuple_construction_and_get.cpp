// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_tuple_get and std_tuple_structured. std::pair works;
// std::tuple construction does not.
#include <tuple>

int main() {
  std::tuple<int, int> pair(1, 2);
  if (std::get<0>(pair) != 1) return 1;
  if (std::get<1>(pair) != 2) return 2;
  std::tuple<int, int, int> triple(1, 2, 3);
  return std::get<2>(triple) - 3;
}
