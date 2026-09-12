namespace NS
{
template<class T> void find(T) {}

template<class T> struct A {};

struct Outer
{
  template<class T> struct B {};
};

struct Derived
{
  template<class T> struct C : public A<T> {};
};
}

template<template<class> class TemplateClass>
struct Holder {};

int main()
{
  Holder<NS::A> holder;
  find(holder);
  Holder<NS::Outer::B> nested;
  find(nested);
  Holder<NS::Derived::C> derived;
  find(derived);
  return 0;
}
