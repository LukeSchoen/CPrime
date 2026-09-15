// EXPECT_COMPILE_FAIL: 1
constexpr const int *escape(int value) { return &value; }
constexpr const int *pointer = escape(3);
