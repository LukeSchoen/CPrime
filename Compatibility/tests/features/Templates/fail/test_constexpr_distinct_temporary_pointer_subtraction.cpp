// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
constexpr const int *address(const Value &value) { return &value.number; }
static_assert(address(Value{1}) - address(Value{2}) == 0);
int main() {}
