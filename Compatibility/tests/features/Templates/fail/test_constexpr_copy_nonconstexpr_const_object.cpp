// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
const Value source{9};
constexpr Value invalid = source;
int main() {}
