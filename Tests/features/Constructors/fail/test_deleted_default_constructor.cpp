// EXPECT_COMPILE_FAIL: 1
struct Value { Value() = delete; };
int main() { Value object; }
