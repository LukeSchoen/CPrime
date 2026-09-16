// EXPECT_COMPILE_FAIL: 1
extern const int value;
template<int &R> struct Ref {};
Ref<value> invalid;
