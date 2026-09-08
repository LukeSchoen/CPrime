// EXPECT_COMPILE_FAIL: 1
int main() { int value; return reinterpret_cast<unsigned char>(&value); }
