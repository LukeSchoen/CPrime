// EXPECT_COMPILE_FAIL: 1
constexpr int invalid() { int value{3.0}; return value; }
static_assert(invalid() == 3);
int main() {}
