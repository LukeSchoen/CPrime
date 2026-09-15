// EXPECT_COMPILE_FAIL: 1
struct Value { constexpr int bad(const int value) const { *(int *)&value = 7; return value; } };
static_assert(Value{}.bad(3) == 7);
