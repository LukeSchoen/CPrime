// EXPECT_COMPILE_ARGS: -std=c++17
// A function parameter naming a variadic class specialization must bind every
// written argument, even when there are more arguments than the template's
// parameter list length.
template<class... Ts> struct vbox { int marker; };

template<class A, class B> int two(vbox<A, B> &) { return 2; }
template<class A, class B, class C> int three(vbox<A, B, C> &) { return 3; }

int main()
{
  vbox<int, char> a;
  vbox<int, char, double> b;
  return two(a) + three(b) == 5 ? 0 : 1;
}
