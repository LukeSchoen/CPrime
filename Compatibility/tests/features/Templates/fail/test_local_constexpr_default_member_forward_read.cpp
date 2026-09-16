// EXPECT_COMPILE_FAIL: 1
struct Value { int first = second; int second = 9; };
int main() { constexpr Value invalid{}; }
