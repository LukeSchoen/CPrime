#include "inherited_virtual_linkage.h"
InheritedVirtual::~InheritedVirtual() { destructor_total += 10; }
void destroy_from_other(InheritedVirtual* object) { object->~InheritedVirtual(); }
