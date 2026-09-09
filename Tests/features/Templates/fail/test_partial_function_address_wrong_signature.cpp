// EXPECT_COMPILE_FAIL: 1
template<class T, class U> int sum(T first, U second) { return first + second; }
int (*function)(int, long) = &sum<double>;
