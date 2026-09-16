// EXPECT_COMPILE_FAIL: 1
constexpr const int *escape() { const int a[2] = {1, 2}; return a; }
constexpr const int *bad = escape();
