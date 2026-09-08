// EXPECT_COMPILE_FAIL: 1
template<class> struct Base {};
struct Derived : Base<int>, Base<double> {};
template<template<class> class C, class T> void select(C<T>*) {}
int main() { Derived object; select(&object); }
