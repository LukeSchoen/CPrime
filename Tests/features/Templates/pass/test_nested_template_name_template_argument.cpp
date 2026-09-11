/* A template template argument may name a member template reached through a
   dependent qualifier chain (`T::template middle<U>::template inner`).
   Substitution hands the nested template itself to the parameter instead of
   demanding its own argument list, so the enclosing specialization can be
   instantiated. */

template<class U> struct outer
{
  template<class V> struct middle
  {
    template<class W> struct inner { V value; };
  };
};

template<template<class> class TT> struct consumer
{
  TT<int> item;
  int size () const { return (int) sizeof (item); }
};

template<class T, class U> struct chain
{
  consumer<T::template middle<U>::template inner> held;
};

int main ()
{
  chain<outer<char>, int> wide;
  if (wide.held.size () != 4)
    return 1;
  return 0;
}
