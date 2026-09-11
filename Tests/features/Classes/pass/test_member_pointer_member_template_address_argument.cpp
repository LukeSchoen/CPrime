template <class T, void (T::* const U)()> struct Good { static int const value = 7; };
struct A {
  template <typename U> void good() {}
  int concrete() { return Good<A, &A::good<int> >::value; }
  template <typename U> int dependent() { return Good<A, &A::good<U> >::value; }
};
int main() { A a; return a.concrete() != 7 || a.dependent<int>() != 7; }
