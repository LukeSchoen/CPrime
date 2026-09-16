// EXPECT_COMPILE_FAIL: 1
int side_effect();
constexpr int bad() { return (side_effect(), 7); }
static_assert(bad() == 7);
