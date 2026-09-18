// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: std_optional_constexpr.
#include <optional>

constexpr int contained() {
  std::optional<int> value{5};
  if (!value.has_value()) return -1;
  return *value;
}

constexpr std::optional<int> empty{};

static_assert(contained() == 5, "optional holds a value in a constant expression");
static_assert(!empty.has_value(), "empty optional");

int main() { return 0; }
