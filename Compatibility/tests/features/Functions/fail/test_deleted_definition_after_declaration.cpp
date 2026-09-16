// EXPECT_COMPILE_FAIL: 1
int unavailable(int);
int unavailable(int) = delete;
int main() { return 0; }
