// EXPECT_COMPILE_FAIL: 1
template<class T> int missing(T x) { return x.no_such_member(); }
template int missing<int>(int);
