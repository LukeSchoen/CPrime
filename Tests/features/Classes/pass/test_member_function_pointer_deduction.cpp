struct A { double f() { return 1.5; } };
struct B { int g(char c) { return c + 1; } };

template <class R, class T>
R call_none(T &object, R (T::*member)()) { return (object.*member)(); }

template <class R, class T, class U>
R call_one(T &object, R (T::*member)(U), U argument) {
  return (object.*member)(argument);
}

template <class R, class T>
R call_none_named(T &object, R (T::*named)()) { return (object.*named)(); }

int main() {
  A a;
  B b;
  if (call_none(a, &A::f) != 1.5) return 1;
  if (call_none_named(a, &A::f) != 1.5) return 2;
  if (call_one(b, &B::g, (char)3) != 4) return 3;
  if (call_one<int, B, char>(b, &B::g, (char)4) != 5) return 4;
  return 0;
}
