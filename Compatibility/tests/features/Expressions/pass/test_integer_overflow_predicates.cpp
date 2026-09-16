static_assert(__builtin_add_overflow_p(127, 1, (signed char)0), "narrow signed");
static_assert(!__builtin_add_overflow_p(~0ULL, -1LL, 0ULL), "mixed signs");
static_assert(__builtin_sub_overflow_p(0U, 1U, 0U), "unsigned negative");
static_assert(__builtin_mul_overflow_p(~0ULL, 2ULL, 0ULL), "wide product");
static_assert(!__builtin_sub_overflow_p(-9223372036854775807LL - 1, 0, 0LL), "minimum");
struct Bits { signed int narrow : 3; unsigned int small : 2; };
template<int N> struct Result { static const int value = N; };
static_assert(Result<__builtin_add_overflow_p(127, 1, (signed char)0) + 1>::value == 2,
              "builtin template value argument");
int main() {
    volatile int left = 2, right = 3;
    Bits bits = {};
    if (!__builtin_add_overflow_p(left, right, bits.narrow)) return 1;
    if (__builtin_sub_overflow_p(left, right, bits.narrow)) return 2;
    if (!__builtin_mul_overflow_p(left, right, bits.small)) return 3;
    volatile unsigned long long maximum = ~0ULL;
    volatile long long negative = -1;
    if (__builtin_add_overflow_p(maximum, negative, 0ULL)) return 4;
    if (!__builtin_sub_overflow_p(maximum, negative, 0ULL)) return 5;
    if (!__builtin_mul_overflow_p(maximum, 2ULL, 0ULL)) return 6;
    int effects = 0;
    if (__builtin_add_overflow_p(1, 2, ++effects) || effects != 1) return 7;
}
