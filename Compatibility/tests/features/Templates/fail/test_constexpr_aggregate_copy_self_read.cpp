// EXPECT_COMPILE_FAIL: 1
struct Value { int member; };
constexpr int bad() {
  Value value = value;
  return value.member;
}
static_assert(bad() == 0);
