// EXPECT_COMPILE_FAIL: 1
constexpr int second = 7;
constexpr int bad() { int first = 3, second = second; return first + second; }
static_assert(bad() == 10);
