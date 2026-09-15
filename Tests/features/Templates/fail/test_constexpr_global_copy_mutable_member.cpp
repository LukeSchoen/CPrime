// EXPECT_COMPILE_FAIL: 1
struct Value { mutable int number; };
constexpr Value source{9};
constexpr Value invalid = source;
int main() {}
