// EXPECT_COMPILE_FAIL: 1
template<class T = int> struct Value;
template<class T = int> struct Value {};
