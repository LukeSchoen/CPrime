// EXPECT_COMPILE_FAIL: 1
int select(int) = delete;
int select(double) { return 0; }
int main() { return select(1); }
