// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
constexpr const int *address(const Value &value) { return &value.number; }
constexpr const int *invalid = address(Value{4});
int main() {}
