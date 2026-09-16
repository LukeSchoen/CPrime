#include "namespace_constant_linkage.h"
const int values::external_number = 14;
const int Holder::number = 15;
const volatile int volatile_number = 16;
const int *other_private_number() { return &private_number; }
const int *other_private_numbers() { return private_numbers; }
const int *const *other_private_pointer() { return &private_pointer; }
const int *other_nested_private_number() { return &values::private_number; }
const int *other_shared_number() { return &values::shared_number; }
int other_external_number() { return values::external_number + Holder::number; }
