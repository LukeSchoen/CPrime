// EXPECT_COMPILE_FAIL: 1
constexpr int operator "" _raw(const char *) { return 1; }
int value = 0x1.2_raw;
