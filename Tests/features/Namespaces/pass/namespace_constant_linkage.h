#ifndef NAMESPACE_CONSTANT_LINKAGE_H
#define NAMESPACE_CONSTANT_LINKAGE_H
const int private_number = 11;
const int private_numbers[] = {17, 18};
const int *const private_pointer = &private_number;
namespace values {
  const int private_number = 12;
  inline const int shared_number = 13;
  extern const int external_number;
}
struct Holder { static const int number; };
extern const volatile int volatile_number;
const int *other_private_number();
const int *other_private_numbers();
const int *const *other_private_pointer();
const int *other_nested_private_number();
const int *other_shared_number();
int other_external_number();
#endif
