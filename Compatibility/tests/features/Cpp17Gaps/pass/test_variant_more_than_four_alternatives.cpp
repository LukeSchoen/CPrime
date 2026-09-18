// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_variant_arity. The library variant is a four-parameter
// template, so a fifth alternative has no parameter to bind to.
#include <variant>

int main() {
  std::variant<int, char, long, short, unsigned> value;
  value = 3;
  if (value.index() != 0) return 1;
  if (!std::holds_alternative<int>(value)) return 2;
  return std::get<int>(value) - 3;
}
