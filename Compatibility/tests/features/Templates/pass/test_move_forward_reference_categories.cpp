// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <utility>
struct Value { int value; };
int category(Value&) { return 1; }
int category(const Value&) { return 2; }
int category(Value&&) { return 3; }
int category(const Value&&) { return 4; }
template<class T> int relay(T&& value) { return category(std::forward<T>(value)); }
int main() {
  Value value = {7};
  const Value constant = {11};
  if (relay(value) != 1 || relay(constant) != 2) return 1;
  if (relay(std::move(value)) != 3 || relay(std::move(constant)) != 4) return 2;
  if (category(std::forward<Value>(Value{9})) != 3) return 3;
  if (&std::forward<Value&>(value) != &value) return 4;
  return std::move(value).value != 7 || std::move(constant).value != 11;
}
