// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_variant_get_if.
#include <variant>

int main() {
  std::variant<int, char> value;
  value = 7;
  int *integer = std::get_if<int>(&value);
  if (integer == 0 || *integer != 7) return 1;
  return std::get_if<char>(&value) == 0 ? 0 : 2;
}
