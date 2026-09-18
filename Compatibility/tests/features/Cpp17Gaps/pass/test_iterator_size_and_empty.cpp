// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_size_data_empty. <iterator> declares std::size but never
// defines it, so the program links against an undefined symbol.
#include <array>
#include <iterator>

int main() {
  std::array<int, 2> values{{1, 2}};
  if (std::size(values) != 2) return 1;
  if (std::empty(values)) return 2;
  std::array<int, 0> none{};
  return std::empty(none) ? 0 : 3;
}
