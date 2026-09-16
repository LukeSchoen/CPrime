// EXPECT_EXIT: 0
// EXPECT_COMPILE_ONLY: 1
// Naming a class template specialization requires only its type identity, so
// `extern B<int> b;` must not complete `B<int>` (whose base `A<int>` is an
// incomplete specialization) and `::f(b)` must deduce the one-argument
// overload without instantiating it either.
template<class T> struct A;
template<class T> struct B : A<T> { };

template<class T> void f(A<T>&, int); // #1
template<class T> void f(B<T>&);      // #2

int main() {
  extern B<int> b;
  ::f(b);
  return 0;
}
