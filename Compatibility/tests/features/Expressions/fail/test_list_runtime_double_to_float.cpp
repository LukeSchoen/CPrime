// EXPECT_COMPILE_FAIL: 1
float convert(double source) { float value{source}; return value; }
int main() { return convert(1.0) != 1.0f; }
