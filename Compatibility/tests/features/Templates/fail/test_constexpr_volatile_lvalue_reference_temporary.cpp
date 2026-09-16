// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
constexpr int invalid() { const volatile Value &alias = Value{7}; return 7; }
static_assert(invalid() == 7);
int main() {}
