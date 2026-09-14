// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
struct Empty {};
int main() { Empty empty; auto [] = empty; }
