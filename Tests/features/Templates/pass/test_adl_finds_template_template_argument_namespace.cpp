namespace NS
{
template<class T> void find(T) {}

template<class T> struct A {};
}

template<template<class> class TemplateClass>
struct Holder {};

int main()
{
  Holder<NS::A> holder;
  find(holder);
  return 0;
}
