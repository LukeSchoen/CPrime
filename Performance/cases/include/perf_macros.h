/* Generated-style header for c.preprocessor.heavy. Kept deliberately dense:
   nested object-like and function-like macros, token pasting and conditional
   chains are what the preprocessor cases are meant to measure. */

#ifndef PERF_MACROS_H
#define PERF_MACROS_H

#define PERF_LEVEL 4

#define PERF_JOIN_(a, b) a##b
#define PERF_JOIN(a, b) PERF_JOIN_(a, b)

#define PERF_COUNTER(n)                                  \
  static int PERF_JOIN(perf_counter_, n)(int x)          \
  {                                                      \
    int acc = x + (n);                                   \
    acc = acc * 3 + (n) * 7;                             \
    acc ^= acc >> 4;                                     \
    return acc;                                          \
  }

#define PERF_REPEAT_1(m) m(1)
#define PERF_REPEAT_2(m) PERF_REPEAT_1(m) m(2)
#define PERF_REPEAT_3(m) PERF_REPEAT_2(m) m(3)
#define PERF_REPEAT_4(m) PERF_REPEAT_3(m) m(4)
#define PERF_REPEAT_5(m) PERF_REPEAT_4(m) m(5)
#define PERF_REPEAT_6(m) PERF_REPEAT_5(m) m(6)
#define PERF_REPEAT_7(m) PERF_REPEAT_6(m) m(7)
#define PERF_REPEAT_8(m) PERF_REPEAT_7(m) m(8)
#define PERF_REPEAT(n, m) PERF_REPEAT_##n(m)

#define PERF_SELECT_2(s) (perf_counter_1(s) + perf_counter_2(s))
#define PERF_SELECT_3(s) (PERF_SELECT_2(s) + perf_counter_3(s))
#define PERF_SELECT_4(s) (PERF_SELECT_3(s) + perf_counter_4(s))
#define PERF_SELECT_5(s) (PERF_SELECT_4(s) + perf_counter_5(s))

#define PERF_FLAG_A 1u
#define PERF_FLAG_B (PERF_FLAG_A << 1)
#define PERF_FLAG_C (PERF_FLAG_B << 1)
#define PERF_FLAG_D (PERF_FLAG_C << 1)
#define PERF_FLAG_ALL (PERF_FLAG_A | PERF_FLAG_B | PERF_FLAG_C | PERF_FLAG_D)

#if PERF_LEVEL >= 4
# define PERF_EXTRA_FLAGS PERF_FLAG_ALL
#elif PERF_LEVEL >= 2
# define PERF_EXTRA_FLAGS (PERF_FLAG_A | PERF_FLAG_B)
#else
# define PERF_EXTRA_FLAGS (PERF_FLAG_A)
#endif

#endif /* PERF_MACROS_H */
