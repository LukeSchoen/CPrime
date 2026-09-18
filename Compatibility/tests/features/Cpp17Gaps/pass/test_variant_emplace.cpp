// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_variant_emplace.
#include <variant>

int main() {
  std::variant<int, char> value;
  value.emplace<int>(9);
  if (!std::holds_alternative<int>(value)) return 1;
  if (std::get<int>(value) != 9) return 2;
  value.emplace<char>('z');
  return std::holds_alternative<char>(value) ? 0 : 3;
}
