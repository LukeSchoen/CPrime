// EXPECT_COMPILE_FAIL: 1
int unavailable(int) = delete;
int unavailable(int value) { return value; }
int main() { return 0; }
