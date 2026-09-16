// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <stdatomic.h>

#define TEST_WIDTH(TYPE, NAME) \
static int NAME(void) { \
    TYPE object = 0, desired = (TYPE)0xa5, observed = 0, expected; \
    __atomic_store(&object, &desired, __ATOMIC_RELEASE); \
    __atomic_load(&object, &observed, __ATOMIC_ACQUIRE); \
    if (observed != desired) return 1; \
    expected = 3; desired = 9; \
    if (__atomic_compare_exchange(&object, &expected, &desired, 0, \
                                  __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE)) return 2; \
    if (expected != (TYPE)0xa5 || object != (TYPE)0xa5) return 3; \
    if (!__atomic_compare_exchange(&object, &expected, &desired, 1, \
                                   __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) return 4; \
    if (expected != (TYPE)0xa5 || object != 9) return 5; \
    desired = (TYPE)~(TYPE)0; \
    __atomic_exchange(&object, &desired, &observed, __ATOMIC_SEQ_CST); \
    if (observed != 9 || object != desired) return 6; \
    if (__atomic_add_fetch(&object, (TYPE)2, __ATOMIC_RELAXED) != 1) return 7; \
    if (__atomic_fetch_sub(&object, (TYPE)1, __ATOMIC_SEQ_CST) != 1) return 8; \
    if (__atomic_or_fetch(&object, (TYPE)0x35, __ATOMIC_RELAXED) != 0x35) return 9; \
    if (__atomic_xor_fetch(&object, (TYPE)0x30, __ATOMIC_RELAXED) != 5) return 10; \
    if (__atomic_and_fetch(&object, (TYPE)3, __ATOMIC_RELAXED) != 1) return 11; \
    if (__atomic_nand_fetch(&object, (TYPE)1, __ATOMIC_RELAXED) != (TYPE)~1) return 12; \
    return 0; \
}
TEST_WIDTH(unsigned char, test_byte)
TEST_WIDTH(unsigned short, test_short)
TEST_WIDTH(unsigned int, test_int)
TEST_WIDTH(unsigned long long, test_long_long)

int main(void) {
    unsigned long long wide = 0, desired = 0xfedcba9876543210ull, read = 0;
    unsigned int stores[2] = {0, 0};
    int index = 0, value = 10, order = __ATOMIC_RELAXED;
    atomic_flag flag = ATOMIC_FLAG_INIT;
    if (test_byte() || test_short() || test_int() || test_long_long()) return 1;
    __atomic_store(&wide, &desired, __ATOMIC_SEQ_CST);
    __atomic_load(&wide, &read, __ATOMIC_SEQ_CST);
    if (read != desired) return 2;
    __atomic_store_n(&stores[index++], value++, order++);
    if (index != 1 || value != 11 || order != 1 || stores[0] != 10 || stores[1]) return 3;
    if (atomic_flag_test_and_set(&flag)) return 4;
    if (!atomic_flag_test_and_set_explicit(&flag, memory_order_acquire)) return 5;
    atomic_flag_clear_explicit(&flag, memory_order_release);
    if (atomic_flag_test_and_set(&flag)) return 6;
    atomic_flag_clear(&flag);
    atomic_thread_fence(memory_order_seq_cst);
    atomic_signal_fence(memory_order_acquire);
    if (!__atomic_is_lock_free(8, &wide)) return 7;
    if (__atomic_is_lock_free(8, (char *)&wide + 1)) return 8;
    return 0;
}
