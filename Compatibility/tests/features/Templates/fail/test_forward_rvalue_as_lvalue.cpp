// EXPECT_COMPILE_FAIL: 1
#include <utility>
int main() { return std::forward<int&>(2); }
