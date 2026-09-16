// EXPECT_COMPILE_FAIL: 1
constexpr double invalid() { double value{9007199254740993LL}; return value; }
static_assert(invalid() == 9007199254740992.0);
int main() {}
