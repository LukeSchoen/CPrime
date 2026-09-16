// EXPECT_EXIT: 0
template<class T> class leaf {
public:
  int value() { return sizeof(T); }
};

template<template<class> class Base, class T> class wrapper : Base<T> {
public:
  int value() { return Base<T>::value(); }
};

struct derived : wrapper<leaf, int> {
  int value() { return wrapper< ::leaf, int>::value(); }
};

int main()
{
  derived value;
  return value.value() == sizeof(int) ? 0 : 1;
}
