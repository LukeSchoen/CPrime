constexpr int add(int left, int right = 3) { return left + right; }
constexpr int nested(int value) { return add(value, add(value)); }
constexpr double half(double value) { return value / 2; }
constexpr unsigned long long identity(unsigned long long value) { return value; }
constexpr int local(int value) { int doubled = value * 2; return doubled + 1; }
struct Bits { int value : 3; };
constexpr bool field_overflow(int value) { Bits bits = {}; return __builtin_add_overflow_p(value, 1, bits.value); }
static_assert(add(4) == 7, "default parameter");
static_assert(nested(4) == 11, "nested evaluation");
static_assert(half(9.0) == 4.5, "floating parameter");
static_assert(identity(1ULL << 50) == (1ULL << 50), "wide parameter");
static_assert(local(4) == 9, "local scalar");
static_assert(field_overflow(3) && !field_overflow(2), "local bit-field type");
int main() { return nested(4) != 11; }
