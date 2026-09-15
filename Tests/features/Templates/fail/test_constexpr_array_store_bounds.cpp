// EXPECT_COMPILE_FAIL: 1
constexpr int bad() { int values[2] = {1, 2}; values[2] = 7; return 7; }
static_assert(bad() == 7);
