// EXPECT_COMPILE_FAIL: 1
constexpr int invalid() { unsigned char value{256}; return value; }
static_assert(invalid() == 0);
int main() {}
