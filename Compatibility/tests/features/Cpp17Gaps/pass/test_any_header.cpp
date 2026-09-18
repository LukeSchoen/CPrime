// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_any. <any> is missing from the runtime.
#include <any>

int main() {
  std::any value;
  if (value.has_value()) return 1;
  value = 3;
  if (!value.has_value()) return 2;
  return std::any_cast<int>(value) - 3;
}
