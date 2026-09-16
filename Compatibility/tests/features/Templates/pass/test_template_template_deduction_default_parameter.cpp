// EXPECT_EXIT: 0
struct Block { static const unsigned dimension = 1; };
template<class T, unsigned N = T::dimension> struct View {};
template<template<class> class V, class T> int inspect(const V<T>&) { return T::dimension; }
int main() { View<Block> v; return inspect(v) != 1; }
