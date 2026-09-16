// EXPECT_COMPILE_FAIL: 1
template<class T> auto* invalid(T) {}
int main() { invalid(3); }
