// EXPECT_COMPILE_FAIL: 1
struct Value { int first; int second; };
int Value::*external = &Value::second;
constexpr bool run() {
  int Value::*pointer = &Value::first;
  pointer = external;
  return true;
}
static_assert(run());
