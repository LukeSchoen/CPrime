// EXPECT_COMPILE_FAIL: 1
constexpr float invalid() { float value{16777217}; return value; }
static_assert(invalid() == 16777216.0f);
int main() {}
