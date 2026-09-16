static_assert(__builtin_clzg((unsigned char)1) == 7, "unpromoted width");
static_assert(__builtin_clrsbg((signed char)-1) == 7, "sign bits");
static_assert(__builtin_ctzg(0U, -9) == -9, "zero result");
static_assert(__builtin_popcountg(~0ULL) == 64, "wide count");
int main() {
    volatile unsigned char byte = 8;
    volatile unsigned short word = 0;
    volatile signed char negative = -2;
    volatile unsigned long long wide = 1ULL << 60;
    if (__builtin_clzg(byte) != 4 || __builtin_ctzg(byte) != 3) return 1;
    if (__builtin_clzg(word, -7) != -7 || __builtin_ctzg(word, 16) != 16) return 2;
    if (__builtin_clrsbg(negative) != 6 || __builtin_ffsg(negative) != 2) return 3;
    if (__builtin_parityg(byte) != 1 || __builtin_popcountg(wide) != 1) return 4;
    if (__builtin_clzg(wide) != 3 || __builtin_ctzg(wide) != 60) return 5;
    unsigned char increment = 1;
    if (__builtin_popcountg(increment++) != 1 || increment != 2) return 6;
}
