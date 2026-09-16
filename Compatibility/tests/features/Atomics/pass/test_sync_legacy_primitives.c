// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
// The legacy full-barrier primitives must reach the runtime helper for the
// width of the object they are handed, and must report the value they
// replaced.
int main(void) {
    volatile unsigned char byte = 7;
    volatile unsigned short half = 7;
    volatile unsigned int word = 7;
    volatile unsigned long long wide = 7;

    __sync_synchronize();

    __sync_lock_release(&byte);
    __sync_lock_release(&half);
    __sync_lock_release(&word);
    __sync_lock_release(&wide);
    if (byte != 0 || half != 0 || word != 0 || wide != 0) return 1;

    if (__sync_fetch_and_add(&byte, 3) != 0 || byte != 3) return 2;
    if (__sync_add_and_fetch(&half, 4) != 4 || half != 4) return 3;
    if (__sync_or_and_fetch(&word, 0x30) != 0x30) return 4;
    if (__sync_xor_and_fetch(&word, 0x0c) != 0x3c) return 5;
    if (__sync_and_and_fetch(&word, 0x38) != 0x38) return 6;
    if (__sync_nand_and_fetch(&wide, 0xff) != ~(unsigned long long)0) return 7;
    if (__sync_fetch_and_sub(&wide, 1) != ~(unsigned long long)0) return 8;

    if (__sync_lock_test_and_set(&word, 0x1234) != 0x38) return 9;
    if (word != 0x1234) return 10;
    if (!__sync_bool_compare_and_swap(&word, 0x1234, 0x4321)) return 11;
    if (word != 0x4321) return 12;
    if (__sync_bool_compare_and_swap(&word, 0x1234, 0x4321)) return 13;
    if (__sync_val_compare_and_swap(&word, 0x4321, 1) != 0x4321) return 14;
    if (__sync_val_compare_and_swap(&word, 0x4321, 2) != 1) return 15;
    if (!__sync_bool_compare_and_swap(&byte, 3, 0)) return 16;
    if (__sync_val_compare_and_swap(&half, 4, 6) != 4 || half != 6) return 17;
    return 0;
}
