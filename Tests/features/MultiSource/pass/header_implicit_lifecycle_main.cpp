// EXPECT_EXIT: 0
#include "header_implicit_lifecycle.h"

int lifecycle_total;
int main() {
  if (lifecycle_from_other_unit() != 7 || lifecycle_total != 7)
    return 1;
  {
    LifecycleOwner owner(11);
    if (owner.leaf.value != 11)
      return 2;
  }
  return lifecycle_total == 18 ? 0 : 3;
}
