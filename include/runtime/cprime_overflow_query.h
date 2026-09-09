#ifndef CPRIME_OVERFLOW_QUERY_H
#define CPRIME_OVERFLOW_QUERY_H
/* Compare the mathematical result with the destination range without
   overflowing the host's signed arithmetic or requiring a wider integer. */
static int cpc_overflow_query(unsigned long long left, unsigned long long right,
                              int flags, int width, int operation)
{
    int left_negative = (flags & 1) && (left >> 63);
    int right_negative = (flags & 2) && (right >> 63);
    int negative;
    unsigned long long magnitude, limit;
    if (left_negative) left = 0ULL - left;
    if (right_negative) right = 0ULL - right;
    if (operation == 2) {
        if (right && left > ~0ULL / right) return 1;
        magnitude = left * right;
        negative = left_negative != right_negative;
    } else {
        if (operation == 1 && right) right_negative = !right_negative;
        if (left_negative == right_negative) {
            magnitude = left + right;
            if (magnitude < left) return 1;
            negative = left_negative;
        } else if (left >= right) {
            magnitude = left - right;
            negative = left_negative;
        } else {
            magnitude = right - left;
            negative = right_negative;
        }
    }
    if (flags & 4) {
        if (negative && magnitude) return 1;
        limit = width == 64 ? ~0ULL : (1ULL << width) - 1;
    } else {
        limit = 1ULL << (width - 1);
        if (!negative) --limit;
    }
    return magnitude > limit;
}
#endif
