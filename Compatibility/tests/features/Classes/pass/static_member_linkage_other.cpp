#include "static_member_linkage.h"
int private_value(int) { return 100; }
int member_linkage::Service::private_value(int value) { return value + 2; }
int member_linkage::Service::protected_value(short value) { return value + 3; }
int member_linkage::Service::value(int value) {
  return private_value(value) + protected_value(4);
}
double member_linkage::Service::value(double value) { return value + 0.5; }
int member_linkage::Service::Nested_Type::value(int value) { return value + 5; }
