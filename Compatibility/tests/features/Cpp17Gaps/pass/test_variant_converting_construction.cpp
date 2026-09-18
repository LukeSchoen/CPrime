// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_variant_converting_ctor. Only default construction and
// assignment work today; construction from a value must select the alternative.
#include <variant>

int main() {
  std::variant<int, double, char> value{2.5};
  if (value.index() != 1) return 1;
  return std::get<double>(value) == 2.5 ? 0 : 2;
}
