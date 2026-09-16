// EXPECT_COMPILE_FAIL: 1
struct Value { ~Value() = delete; };
int main() { (void)new Value[0]; }
