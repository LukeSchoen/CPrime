#pragma once
extern int destructor_total;
struct VirtualRoot {
  virtual ~VirtualRoot() { destructor_total += 1; }
};
struct Intermediate : VirtualRoot {};
struct InheritedVirtual : Intermediate {
  ~InheritedVirtual();
};
void destroy_from_other(InheritedVirtual* object);
