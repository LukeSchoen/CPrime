// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
int main() { int values[1] = {3}; auto [value,] = values; }
