// EXPECT_COMPILE_FAIL: 1
struct Value { mutable int number; };
constexpr Value value{9};
constexpr const int *pointer = &value.number;
static_assert(*pointer == 9);
int main() {}
