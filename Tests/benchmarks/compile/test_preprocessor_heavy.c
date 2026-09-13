// PERF_NAME: c.preprocessor.heavy
// PERF_ARGS: -ITests/benchmarks/compile/include
/* Preprocessing volume: nested macro expansion, conditional chains and a
   generated header included through the quoted search path. */

#include "perf_macros.h"

PERF_REPEAT(8, PERF_COUNTER)

static int dispatch(int selector)
{
  int total = 0;
#if PERF_LEVEL >= 3
  total += PERF_SELECT_3(selector);
#else
  total += PERF_SELECT_2(selector);
#endif
#if PERF_LEVEL >= 4
  total += PERF_SELECT_4(selector);
#endif
#if PERF_LEVEL >= 5
  total += PERF_SELECT_5(selector);
#endif
  return total;
}

int main(void)
{
  int i;
  int total = 0;
  for (i = 0; i < 64; i++)
    total += dispatch(i);
  return total == 0 ? 1 : 0;
}
