// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -O2
#include <string.h>

static unsigned char source[160], destination[160];
#define CHECK(N) do { \
    unsigned i; \
    for (i = 0; i < 160; ++i) destination[i] = 0xa5; \
    if (memcpy(destination + 1, source + 3, N) != destination + 1) return 1; \
    for (i = 0; i < 160; ++i) \
        if (destination[i] != ((i >= 1 && i < 1 + N) ? source[i + 2] : 0xa5)) return 2; \
    if (memset(destination + 1, 0x1234, N) != destination + 1) return 3; \
    for (i = 1; i < 1 + N; ++i) if (destination[i] != 0x34) return 4; \
    if (destination[0] != 0xa5 || destination[1 + N] != 0xa5) return 5; \
    memset(destination + 1, 0, N); \
    for (i = 1; i < 1 + N; ++i) if (destination[i]) return 6; \
} while (0)

int main(void) {
    unsigned i;
    int d = 0, s = 0;
    struct Record { unsigned char bytes[63]; } a, b;
    for (i = 0; i < 160; ++i) source[i] = (unsigned char)(i * 19);
    CHECK(0); CHECK(1); CHECK(2); CHECK(3); CHECK(4); CHECK(7); CHECK(8);
    CHECK(9); CHECK(15); CHECK(16); CHECK(17); CHECK(31); CHECK(32);
    CHECK(33); CHECK(48); CHECK(63); CHECK(64); CHECK(127); CHECK(128); CHECK(129);
    if (memcpy(destination + d++, source + s++, 17) != destination) return 7;
    if (d != 1 || s != 1) return 8;
    for (i = 0; i < 63; ++i) a.bytes[i] = (unsigned char)(i * 3);
    b = a; a = a;
    for (i = 0; i < 63; ++i) if (a.bytes[i] != b.bytes[i]) return 9;
    return 0;
}
