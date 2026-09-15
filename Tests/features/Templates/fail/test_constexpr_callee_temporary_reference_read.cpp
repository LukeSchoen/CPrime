// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
constexpr const int &member(const Value &value) { return value.number; }
constexpr const int &expired() { return member(Value{9}); }
static_assert(expired() == 9);
int main() {}
