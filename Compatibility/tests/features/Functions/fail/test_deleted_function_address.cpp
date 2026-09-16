// EXPECT_COMPILE_FAIL: 1
int unavailable(int) = delete;
int unavailable(double) { return 0; }
int (*pointer)(int) = &unavailable;
int main() { return 0; }
