// EXPECT_COMPILE_FAIL: 1
int runtime_call();
constexpr int bad() { for (; runtime_call(), false;) {} return 7; }
static_assert(bad() == 7);
