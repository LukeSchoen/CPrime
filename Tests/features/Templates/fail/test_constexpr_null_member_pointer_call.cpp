// EXPECT_COMPILE_FAIL: 1
struct Value { constexpr int get() const { return 7; } };
constexpr int bad() {
  Value value{};
  int (Value::*pointer)() const = nullptr;
  ((value.*pointer)(), 0);
  return 7;
}
static_assert(bad() == 7);
