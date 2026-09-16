// EXPECT_COMPILE_FAIL: 1
struct Cell {};
template<template<class> class C> struct Owner {};
Owner<Cell> invalid;
