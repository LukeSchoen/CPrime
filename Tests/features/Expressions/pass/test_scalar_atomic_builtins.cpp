enum State { idle, ready };
int main() {
  int value = 3, expected = 3;
  if (__atomic_exchange_n(&value, 7, __ATOMIC_SEQ_CST) != 3) return 1;
  if (__atomic_compare_exchange_n(&value, &expected, 11, false,
        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE) || expected != 7) return 2;
  if (!__atomic_compare_exchange_n(&value, &expected, 11, false,
        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) return 3;
  if (__atomic_load_n(&value, __ATOMIC_ACQUIRE) != 11) return 4;
  __atomic_store_n(&value, 13, __ATOMIC_RELEASE);
  State state = idle, old = idle;
  if (!__atomic_compare_exchange_n(&state, &old, ready, false,
        __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)) return 5;
  int *pointer = 0;
  if (__atomic_exchange_n(&pointer, &value, __ATOMIC_SEQ_CST) != 0) return 6;
  return *__atomic_load_n(&pointer, __ATOMIC_ACQUIRE) != 13 || state != ready;
}
