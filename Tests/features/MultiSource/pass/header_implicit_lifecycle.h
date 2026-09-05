#ifndef TEST_HEADER_IMPLICIT_LIFECYCLE_H
#define TEST_HEADER_IMPLICIT_LIFECYCLE_H

extern int lifecycle_total;

struct LifecycleLeaf {
  int value;
  template<class T> LifecycleLeaf(const T &input) : value(input) {}
  ~LifecycleLeaf() { lifecycle_total += value; }
};

// The implicit destructor is first needed inside each translation unit's
// caller function, rather than while parsing this class declaration.
struct LifecycleOwner {
  LifecycleLeaf leaf;
  LifecycleOwner(int input) : leaf(input) {}
};

int lifecycle_from_other_unit();
#endif
