// EXPECT_COMPILE_FAIL: 1
int global;
constexpr int bad() { global = 7; return 1; }
static_assert(bad() == 1);
