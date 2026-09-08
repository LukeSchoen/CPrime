template<class> struct One {};
template<class, class> struct Two {};
template<int> struct Number {};
template<template<class> class C, class T> int select(T) { return 1; }
template<template<class, class> class C, class T> int select(T) { return 2; }
template<template<int> class C, class T> int select(T) { return 3; }
int main() { return select<One>(0) != 1 || select<Two>(0) != 2 || select<Number>(0) != 3; }
