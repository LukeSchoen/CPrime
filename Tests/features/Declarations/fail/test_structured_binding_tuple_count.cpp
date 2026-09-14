// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
#include <tuple>
int main() { std::tuple<int> tuple(3); auto &[first, second] = tuple; }
