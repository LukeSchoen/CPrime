// EXPECT_COMPILE_FAIL: 1
#include <initializer_list>
struct Item { explicit Item(int, int) {} };
int main() { std::initializer_list<Item> values = {{1, 2}}; }
