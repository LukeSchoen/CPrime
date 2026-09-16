// EXPECT_COMPILE_FAIL: 1
int main() { double value; return const_cast<int*>(&value) != 0; }
