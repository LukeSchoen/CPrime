// EXPECT_COMPILE_FAIL: 1
template<class T> int pick(T);
template<class T> int pick(T, T);
auto pointer = pick<int>;
