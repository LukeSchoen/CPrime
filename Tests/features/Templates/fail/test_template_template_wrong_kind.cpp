// EXPECT_COMPILE_FAIL: 1
template<int N> struct Value {};
template<template<class> class C> struct Owner { C<int> cell; };
Owner<Value> invalid;
