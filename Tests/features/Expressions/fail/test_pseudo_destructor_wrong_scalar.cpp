// EXPECT_COMPILE_FAIL: 1
typedef float Other;
int main() { int value = 0; value.~Other(); }
