// EXPECT_COMPILE_FAIL: 1
void runtime_call();
constexpr void bad() { runtime_call(); }
constexpr int run() { bad(); return 7; }
static_assert(run() == 7);
