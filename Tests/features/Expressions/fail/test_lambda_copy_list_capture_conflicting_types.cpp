// EXPECT_COMPILE_FAIL: 1
#include <initializer_list>
int main() { auto invalid = [values = {1, 2.0}] {}; }
