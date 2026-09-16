// EXPECT_COMPILE_FAIL: 1
template<int *Pointer> struct Value {};
Value<0> invalid;
