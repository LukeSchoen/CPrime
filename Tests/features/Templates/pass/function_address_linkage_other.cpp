#include "function_address_linkage.h"
int address_first(int n) { return n + 7; }
int address_second(int n) { return n * 3; }
int read_address(AddressTag<decltype(&address_first), &address_first> x) { return address_first(x.value); }
int read_address(AddressTag<decltype(&address_second), &address_second> x) { return address_second(x.value); }
