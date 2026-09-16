// EXPECT_COMPILE_FAIL: 1
constexpr int value = 7;
constexpr int f() { int value = value; return value; }
static_assert(f() == 7);
