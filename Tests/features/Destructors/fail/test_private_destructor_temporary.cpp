// EXPECT_COMPILE_FAIL: 1
struct Value { private: ~Value() {} };
int main() { Value{}; }
