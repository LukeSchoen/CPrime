// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -O2
#include <stdint.h>
#include <stdarg.h>

static int table[] = { 7, 11, 19, 31 };
static int calls;
static int choose(int x) { return x < 0 ? -x : x + 7; }
static inline int lookup(unsigned x) {
    if (x >= 4) return -1;
    return table[x];
}
static uint64_t wide(uint64_t x) { return (x >> 33) ^ (x << 7); }
static int loop(int x) { int total = 0; while (x > 0) total += x--; return total; }
static int many(int a, int b, int c, int d, int e, int f) {
    return a + b * 3 + c * 5 + d * 7 + e * 11 + f * 13;
}
static int side(void) { return ++calls; }
static int escaped(int x) { int *p = &x; *p += 9; return x; }
static uint64_t overlap(uint64_t x) {
    union { uint64_t wide; unsigned low[2]; } u;
    u.wide = x; u.low[0] = 17; return u.wide;
}
static int volatile_local(int x) { volatile int v = x; v += 3; return v; }
static int variadic(int n, ...) {
    va_list ap; int total = 0;
    va_start(ap, n); while (n--) total += va_arg(ap, int); va_end(ap);
    return total;
}
int main(void) {
    int i;
    int (*volatile indirect)(int) = choose;
    for (i = -100; i < 100; ++i) {
        if (choose(i) != (i < 0 ? -i : i + 7)) return 1;
        if (indirect(i) != choose(i)) return 2;
        if (lookup((unsigned)i) != ((unsigned)i < 4 ? table[i] : -1)) return 3;
    }
    {
        uint64_t x = UINT64_C(0xfedcba9876543210);
        if (wide(x) != ((x >> 33) ^ (x << 7))) return 4;
    }
    if (loop(20) != 210 || many(1,2,3,4,5,6) != 183) return 5;
    if (choose(side()) != 8 || calls != 1) return 6;
    if (escaped(4) != 13 || volatile_local(5) != 8) return 7;
    if (overlap(UINT64_C(0xabcdef1200000000)) != UINT64_C(0xabcdef1200000011)) return 8;
    if (variadic(6,1,2,3,4,5,6) != 21) return 9;
    return 0;
}
