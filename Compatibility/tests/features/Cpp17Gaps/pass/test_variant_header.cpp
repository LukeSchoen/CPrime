// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_variant. <variant> is missing from the runtime.
#include <variant>

int main() {
  std::variant<int, float> value;
  value = 1;
  if (value.index() != 0) return 1;
  if (!std::holds_alternative<int>(value)) return 2;
  return std::get<int>(value) - 1;
}
