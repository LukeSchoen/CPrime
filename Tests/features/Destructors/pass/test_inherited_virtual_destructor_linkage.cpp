// EXPECT_SOURCES: ["inherited_virtual_linkage_other.cpp"]
#include "inherited_virtual_linkage.h"
#include <new>
int destructor_total;
int main() {
  union Storage { char bytes[sizeof(InheritedVirtual)]; long long alignment; } first, second;
  InheritedVirtual* a = new(first.bytes) InheritedVirtual;
  a->~InheritedVirtual();
  if (destructor_total != 11) return 1;
  InheritedVirtual* b = new(second.bytes) InheritedVirtual;
  destroy_from_other(b);
  return destructor_total != 22;
}
