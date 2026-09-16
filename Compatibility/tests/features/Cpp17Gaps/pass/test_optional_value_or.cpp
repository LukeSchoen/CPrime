// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: std_optional_value_or. Basic optional use works; value_or()
// does not.
#include <optional>

int main() {
  std::optional<int> empty;
  if (empty.value_or(7) != 7) return 1;
  std::optional<int> full(3);
  return full.value_or(7) - 3;
}
