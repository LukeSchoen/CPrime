// EXPECT_COMPILE_FAIL: 1
int main() { auto invalid = [value = 1](auto value) { return value; }; }
