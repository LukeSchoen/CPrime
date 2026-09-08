// EXPECT_COMPILE_FAIL: 1
struct Value { int value; ~Value() {} };
int main() { Value value; return __builtin_bit_cast(int, value); }
