// EXPECT_COMPILE_FAIL: 1
constexpr int values[1] = {7};
constexpr int f() { int values[1] = {values[0]}; return values[0]; }
static_assert(f() == 7);
