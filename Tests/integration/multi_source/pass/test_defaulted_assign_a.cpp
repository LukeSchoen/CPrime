#include "test_defaulted_assign.h"

int assign_value(Box *dst, const Box *src)
{
  *dst = *src;
  return dst->value;
}
