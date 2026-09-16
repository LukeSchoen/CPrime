// EXPECT_COMPILE_FAIL: 1
template<class T> int value(T x) { return 1; }
int before() { return value(2); }
template<> int value(int x) { return 2; }
int main() { return before(); }
