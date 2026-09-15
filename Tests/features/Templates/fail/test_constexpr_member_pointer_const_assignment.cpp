// EXPECT_COMPILE_FAIL: 1
struct Value { int first; int second; };
constexpr bool run() {
  int Value::*const pointer = &Value::first;
  const_cast<int Value::*&>(pointer) = &Value::second;
  return true;
}
static_assert(run());
