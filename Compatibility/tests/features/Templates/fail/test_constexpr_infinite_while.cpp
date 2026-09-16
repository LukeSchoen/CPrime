// EXPECT_COMPILE_FAIL: 1
constexpr int bad() { while (true) {} return 0; }
static_assert(bad() == 0);
