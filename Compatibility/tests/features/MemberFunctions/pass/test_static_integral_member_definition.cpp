// EXPECT_EXIT: 0
struct Limits {
    static const long long timeout = 3000;
    static const long long negative = -1;
    static const unsigned long long wide = 0x123456789abcdef0ULL;
    static const int zero = 0;
    static long long read() { return timeout; }
    long long instance_read() { return timeout; }
};
const long long *before_definition() { return &Limits::timeout; }
const long long *early_address = &Limits::timeout;
const long long Limits::timeout;
const long long Limits::negative;
const unsigned long long Limits::wide;
const int Limits::zero;
int main() {
    volatile const long long *timeout = &Limits::timeout;
    volatile const long long *negative = &Limits::negative;
    volatile const unsigned long long *wide = &Limits::wide;
    volatile const int *zero = &Limits::zero;
    if (*timeout != 3000 || *before_definition() != 3000
        || *early_address != 3000) return 1;
    if (*negative != -1) return 2;
    if (*wide != 0x123456789abcdef0ULL) return 3;
    if (*zero != 0) return 4;
    Limits limits;
    if (Limits::read() != 3000 || limits.instance_read() != 3000) return 6;
    int array[Limits::timeout / 1000];
    if (sizeof(array) != 3 * sizeof(int)) return 5;
    return 0;
}
