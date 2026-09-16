// EXPECT_COMPILE_FAIL: 1
int accept(int &&value) { return value; }
int main() { int value = 9; return accept(value); }
