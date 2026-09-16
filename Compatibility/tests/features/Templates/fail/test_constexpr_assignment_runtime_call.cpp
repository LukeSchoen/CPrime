// EXPECT_COMPILE_FAIL: 1
int side_effect();
constexpr int bad() { int value = 0; value = (side_effect(), 7); return value; }
static_assert(bad() == 7);
