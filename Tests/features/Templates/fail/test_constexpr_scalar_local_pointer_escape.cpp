// EXPECT_COMPILE_FAIL: 1
constexpr const int *escape() { int value = 7; return &value; }
constexpr const int *pointer = escape();
