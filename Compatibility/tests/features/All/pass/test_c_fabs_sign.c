#include <math.h>
#include <stdint.h>
#include <string.h>

int main(void)
{
    const uint32_t inputs_f[] = { UINT32_C(0x80000000), UINT32_C(0x80000001), UINT32_C(0xff800000), UINT32_C(0xffc12345) };
    const uint64_t inputs_d[] = { UINT64_C(0x8000000000000000), UINT64_C(0x8000000000000001), UINT64_C(0xfff0000000000000), UINT64_C(0xfff8123456789abc) };
    unsigned i;
    for (i = 0; i < sizeof(inputs_f) / sizeof(inputs_f[0]); ++i) {
        float value, result;
        uint32_t actual;
        memcpy(&value, &inputs_f[i], sizeof(value));
        result = fabsf(value);
        memcpy(&actual, &result, sizeof(actual));
        if (actual != (inputs_f[i] & UINT32_C(0x7fffffff))) return 1;
    }
    for (i = 0; i < sizeof(inputs_d) / sizeof(inputs_d[0]); ++i) {
        double value, result;
        uint64_t actual;
        memcpy(&value, &inputs_d[i], sizeof(value));
        result = fabs(value);
        memcpy(&actual, &result, sizeof(actual));
        if (actual != (inputs_d[i] & UINT64_C(0x7fffffffffffffff))) return 2;
    }
    if (signbit(fabsl(-0.0L)) || fabsl(-HUGE_VALL) != HUGE_VALL) return 3;
    return 0;
}
