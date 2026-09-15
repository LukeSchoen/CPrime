// EXPECT_COMPILE_FAIL: 1
constexpr int bad(const int value) { *(int *)&value = 7; return value; }
static_assert(bad(3) == 7);
