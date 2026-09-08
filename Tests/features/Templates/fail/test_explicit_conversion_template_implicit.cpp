// EXPECT_COMPILE_FAIL: 1
struct Value { template<class T> explicit operator T() { return T(); } };
int main() { int result = Value(); return result; }
