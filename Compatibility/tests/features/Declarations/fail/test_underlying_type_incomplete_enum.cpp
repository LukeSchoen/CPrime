// EXPECT_COMPILE_FAIL: 1
enum Incomplete;
__underlying_type(Incomplete) invalid;
