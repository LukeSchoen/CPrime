// EXPECT_COMPILE_FAIL: 1
struct Value { template<class T> int read(T) = delete; };
int main() { Value value; return value.read(1); }
