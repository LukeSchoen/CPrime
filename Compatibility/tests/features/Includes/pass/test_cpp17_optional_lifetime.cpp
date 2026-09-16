// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: MSVC STL P0220R1_optional construction/reset and contained lifetime.
#include <optional>
int alive;
struct Value {
  int number;
  Value(int n) : number(n) { ++alive; }
  Value(const Value &other) : number(other.number) { ++alive; }
  ~Value() { --alive; }
};
int main() {
  std::optional<Value> value;
  if (alive || value.has_value()) return 1;
  value.emplace(7);
  if (alive != 1 || value->number != 7) return 2;
  value.reset();
  if (alive || value.has_value()) return 3;
  value.emplace(9);
  value = std::nullopt;
  return alive != 0 || value.has_value();
}
