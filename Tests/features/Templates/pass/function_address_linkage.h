#pragma once
int address_first(int);
int address_second(int);
template<class F, F Fn> struct AddressTag { int value; };
int read_address(AddressTag<decltype(&address_first), &address_first>);
int read_address(AddressTag<decltype(&address_second), &address_second>);
