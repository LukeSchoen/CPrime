// EXPECT_COMPILE_FAIL: 1
template<class T> auto* invalid(T value) { return value; }
int main() { invalid(3); }
