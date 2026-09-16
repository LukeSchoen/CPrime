template<class T> struct Base { int value; };
struct Prefix { int prefix; };
struct Derived : Prefix, Base<int> {};
template<template<class> class C, class T> int read(C<T> value) { return value.value; }
template<class T, class U> struct Pair { int value; };
template<class T> struct Child : Pair<T,T> {};
template<template<class,class> class C, class T, class U> int pointer(C<T,U>* value) {
  return value->value;
}
int main() {
  Derived value; value.prefix = 9; value.value = 7;
  Child<int> child; child.value = 11;
  return read(value) != 7 || pointer(&child) != 11;
}
