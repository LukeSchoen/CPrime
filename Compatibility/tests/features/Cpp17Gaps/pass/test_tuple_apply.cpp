// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_apply_tuple. std::apply is declared but never defined, so
// either standard spelling links against an undefined symbol.
#include <tuple>
#include <utility>

int add(int left, int right) { return left + right; }

int main() {
  std::pair<int, int> arguments = std::make_pair(1, 2);
  return std::apply(add, arguments) - 3;
}
