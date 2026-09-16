// EXPECT_COMPILE_FAIL: 1
struct Value { operator int() { return 3; } };
int main() { return Value().operator long(); }
