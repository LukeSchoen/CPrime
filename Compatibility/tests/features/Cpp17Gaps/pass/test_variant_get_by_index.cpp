// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_variant_get_index. Only the by-type get is provided.
#include <variant>

int main() {
  std::variant<int, char> value;
  value = 7;
  if (std::get<0>(value) != 7) return 1;
  value = 'c';
  return std::get<1>(value) == 'c' ? 0 : 2;
}
