// EXPECT_COMPILE_FAIL: 1
struct Value { Value& operator=(Value&); };
int main() { Value destination; destination = Value(); }
