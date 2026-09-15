// EXPECT_COMPILE_FAIL: 1
struct Value { int first = second; int second = 9; };
static_assert(Value{}.first == 0);
int main() {}
