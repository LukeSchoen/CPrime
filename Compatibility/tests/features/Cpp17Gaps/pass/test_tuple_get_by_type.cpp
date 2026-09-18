// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_tuple_get_type. Element access by type, which requires the
// type to appear exactly once.
#include <tuple>

int main() {
  std::tuple<int, double, char> value{1, 2.5, 'c'};
  if (std::get<double>(value) != 2.5) return 1;
  std::get<double>(value) = 3.5;
  if (std::get<1>(value) != 3.5) return 2;
  const std::tuple<int, double, char> &view = value;
  return std::get<double>(view) == 3.5 ? 0 : 3;
}
