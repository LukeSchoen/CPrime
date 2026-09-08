#include <stddef.h>
#include <float.h>
_Static_assert(__SIZEOF_SHORT__ == sizeof(short), "short size");
_Static_assert(__SIZEOF_FLOAT__ == sizeof(float), "float size");
_Static_assert(__SIZEOF_DOUBLE__ == sizeof(double), "double size");
_Static_assert(__SIZEOF_LONG_DOUBLE__ == sizeof(long double), "long double size");
_Static_assert(__SIZE_MAX__ == (size_t)-1, "size maximum");
_Static_assert(__PTRDIFF_MAX__ == (ptrdiff_t)(__SIZE_MAX__ >> 1), "difference maximum");
char aligned __attribute__((aligned(__BIGGEST_ALIGNMENT__)));
_Static_assert(__ATOMIC_RELAXED == 0 && __ATOMIC_SEQ_CST == 5, "memory orders");
int main(void) {
  return __DBL_MIN__ != DBL_MIN
      || __DBL_MAX__ != DBL_MAX || __DBL_EPSILON__ != DBL_EPSILON
      || __DBL_DENORM_MIN__ <= 0 || __FLT_DENORM_MIN__ <= 0;
}
