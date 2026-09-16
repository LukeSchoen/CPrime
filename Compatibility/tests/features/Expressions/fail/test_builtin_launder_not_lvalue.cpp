// EXPECT_COMPILE_FAIL: 1
int main() { int value; int *pointer = &value; __builtin_launder(pointer) = 0; }
