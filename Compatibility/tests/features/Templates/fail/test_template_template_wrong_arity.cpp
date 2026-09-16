// EXPECT_COMPILE_FAIL: 1
template<class T, class U> struct Pair {};
template<template<class> class C> struct Owner { C<int> cell; };
Owner<Pair> invalid;
