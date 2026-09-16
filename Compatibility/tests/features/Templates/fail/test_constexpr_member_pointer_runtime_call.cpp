// EXPECT_COMPILE_FAIL: 1
struct Value { int runtime() { return 7; } };
constexpr int bad() {
  Value value{};
  auto pointer = &Value::runtime;
  ((value.*pointer)(), 0);
  return 7;
}
static_assert(bad() == 7);
