// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: std_not_fn. The result of std::not_fn must be callable.
#include <functional>

int main() {
  auto inverted = std::not_fn([] { return false; });
  return inverted() ? 0 : 1;
}
