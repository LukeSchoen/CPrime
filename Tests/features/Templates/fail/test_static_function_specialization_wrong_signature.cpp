// EXPECT_COMPILE_FAIL: 1
template<class T> struct Value { static int read(int); };
template<> int Value<int>::read(double) { return 0; }
