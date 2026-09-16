// EXPECT_COMPILE_FAIL: 1
const double value = 1.0;
static_assert(value == 1.0, "ordinary const double is not constexpr");
