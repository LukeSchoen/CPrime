constexpr int operator "" _kind(unsigned long long) { return 1; }
constexpr int operator "" _kind(long double) { return 2; }
constexpr unsigned long long operator "" _identity(unsigned long long v) { return v; }
static_assert(12_kind == 1);
static_assert(1.25_kind == 2);
static_assert(0xA'B_kind == 1);
static_assert(0x1.Ap2_kind == 2);
static_assert(18446744073709551615_identity == 18446744073709551615ULL);
int main() { return 12_kind != 1 || 1.25_kind != 2; }