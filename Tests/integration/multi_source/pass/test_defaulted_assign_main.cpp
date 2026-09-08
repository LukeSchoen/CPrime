#include "test_defaulted_assign.h"

int main(void)
{
  Box a;
  Box b;
  a.value = 7;
  return assign_value(&b, &a) == 7 ? 0 : 1;
}
