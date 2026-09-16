// EXPECT_COMPILE_FAIL: 1
template<class T> auto& invalid(T value) { return value + 1; }
int main() { invalid(3); }
