// EXPECT_COMPILE_FAIL: 1
int main() { int value; return static_cast<long long>(&value); }
