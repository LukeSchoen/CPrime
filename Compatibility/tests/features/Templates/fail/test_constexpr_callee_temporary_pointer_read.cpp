// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
constexpr const int *address(const Value &value) { return &value.number; }
constexpr const int *expired() { return address(Value{9}); }
static_assert(*expired() == 9);
int main() {}
