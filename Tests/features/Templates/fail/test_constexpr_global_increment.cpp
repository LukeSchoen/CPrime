// EXPECT_COMPILE_FAIL: 1
int global = 3;
constexpr int bad() { return (global++, 7); }
static_assert(bad() == 7);
