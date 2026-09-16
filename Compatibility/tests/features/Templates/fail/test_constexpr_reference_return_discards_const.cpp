// EXPECT_COMPILE_FAIL: 1
constexpr int value = 9;
constexpr int &invalid() { return value; }
static_assert(invalid() == 9);
int main() {}
