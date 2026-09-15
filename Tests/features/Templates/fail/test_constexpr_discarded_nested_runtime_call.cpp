// EXPECT_COMPILE_FAIL: 1
int side_effect();
constexpr int inner() { return (side_effect(), 3); }
constexpr int outer() { return (inner(), 7); }
static_assert(outer() == 7);
