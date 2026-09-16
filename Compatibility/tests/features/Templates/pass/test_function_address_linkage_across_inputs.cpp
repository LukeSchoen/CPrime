// EXPECT_SOURCES: ["function_address_linkage_other.cpp"]
#include "function_address_linkage.h"
int main() {
  AddressTag<decltype(&address_first), &address_first> a = {5};
  AddressTag<decltype(&address_second), &address_second> b = {8};
  if (read_address(a) != 12) return 1;
  return read_address(b) != 24;
}
