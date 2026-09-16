template<int N>
union Storage
{
  int whole;
  char bytes[N];
};

template<template<class> class TemplateClass>
void accept(TemplateClass<int>) {}

template<class T>
union Wrapper
{
  T value;
};

int main()
{
  Storage<4> storage;
  storage.whole = 0x01020304;
  if (storage.bytes[0] != char(0x04))
    return 1;
  accept(Wrapper<int>());
  return 0;
}
