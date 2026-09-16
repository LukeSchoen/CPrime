// EXPECT_COMPILE_FAIL: 1
unsigned long long operator "not empty" _bad(unsigned long long x) { return x; }
