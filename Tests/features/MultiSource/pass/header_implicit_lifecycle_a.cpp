#include "header_implicit_lifecycle.h"

int lifecycle_from_other_unit() {
  LifecycleOwner owner(7);
  return owner.leaf.value;
}
