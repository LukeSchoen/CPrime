// EXPECT_COMPILE_FAIL: 1
int main() { float value{1e100}; return value == 0.0f; }
