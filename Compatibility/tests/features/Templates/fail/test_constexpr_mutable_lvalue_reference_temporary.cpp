// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
constexpr int invalid() { Value &alias = Value{7}; return alias.number; }
static_assert(invalid() == 7);
int main() {}
