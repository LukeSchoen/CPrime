// EXPECT_COMPILE_FAIL: 1
constexpr int value = 9;
constexpr int invalid() { const int &alias{value, value}; return alias; }
static_assert(invalid() == 9);
int main() {}
