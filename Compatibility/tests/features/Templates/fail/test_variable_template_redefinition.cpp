// EXPECT_COMPILE_FAIL: 1
template<class T> constexpr int value = 1;
template<class U> constexpr int value = 2;
int main() { return value<int>; }
