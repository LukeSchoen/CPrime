constexpr float exact() { float value{16777216}; return value; }
constexpr float power() { float value{1ULL << 63}; return value; }
constexpr float negative() { float value{-16777216}; return value; }
constexpr double wide() { double value{9007199254740992LL}; return value; }
static_assert(exact() == 16777216.0f);
static_assert(power() == 9223372036854775808.0f);
static_assert(negative() == -16777216.0f);
static_assert(wide() == 9007199254740992.0);
int main() { return exact() != 16777216.0f || negative() != -16777216.0f; }
