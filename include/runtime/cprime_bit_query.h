#ifndef CPRIME_BIT_QUERY_H
#define CPRIME_BIT_QUERY_H
enum { CPC_BIT_CLZ, CPC_BIT_CTZ, CPC_BIT_CLRSB, CPC_BIT_FFS, CPC_BIT_PARITY, CPC_BIT_POPCOUNT };
static int cpc_bit_query(unsigned long long value, int width, int operation, int zero)
{
    unsigned long long mask = width == 64 ? ~0ULL : (1ULL << width) - 1;
    int count = 0;
    value &= mask;
    if (operation == CPC_BIT_CLRSB) {
        if (value & (1ULL << (width - 1))) value ^= mask;
        if (!value) return width - 1;
        while (!(value & (1ULL << (width - 1)))) { ++count; value <<= 1; }
        return count - 1;
    }
    if (operation == CPC_BIT_CLZ) {
        if (!value) return zero;
        while (!(value & (1ULL << (width - 1)))) { ++count; value <<= 1; }
        return count;
    }
    if (operation == CPC_BIT_CTZ || operation == CPC_BIT_FFS) {
        if (!value) return operation == CPC_BIT_FFS ? 0 : zero;
        while (!(value & 1)) { ++count; value >>= 1; }
        return count + (operation == CPC_BIT_FFS);
    }
    while (value) { value &= value - 1; ++count; }
    return operation == CPC_BIT_PARITY ? count & 1 : count;
}
#endif
