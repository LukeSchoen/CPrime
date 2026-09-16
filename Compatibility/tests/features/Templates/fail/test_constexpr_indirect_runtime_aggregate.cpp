// EXPECT_COMPILE_FAIL: 1
struct Value { int member; };
Value runtime(int input) { return Value{input}; }
constexpr int bad() {
  auto pointer = &runtime;
  (pointer(7), 0);
  return 7;
}
static_assert(bad() == 7);
