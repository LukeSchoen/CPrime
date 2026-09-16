// EXPECT_COMPILE_FAIL: 1
constexpr volatile int value = 9;
constexpr const int *pointer = const_cast<const int *>(&value);
static_assert(*pointer == 9);
int main() {}
