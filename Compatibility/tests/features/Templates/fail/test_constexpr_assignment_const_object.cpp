// EXPECT_COMPILE_FAIL: 1
constexpr int bad() { const int value = 1; *(int *)&value = 2; return value; }
static_assert(bad() == 2);
