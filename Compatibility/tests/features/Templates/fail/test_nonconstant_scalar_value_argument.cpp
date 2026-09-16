// EXPECT_COMPILE_FAIL: 1
int value;
template<int N> struct Value {};
Value<value> invalid;
