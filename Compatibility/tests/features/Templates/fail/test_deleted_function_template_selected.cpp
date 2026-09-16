// EXPECT_COMPILE_FAIL: 1
template<class T> int unavailable(T) = delete;
int main() { return unavailable(1); }
