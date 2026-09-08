// EXPECT_COMPILE_FAIL: 1
struct Value { template<class T> friend int hidden(T) { return 1; } };
int main() { return hidden(0); }
