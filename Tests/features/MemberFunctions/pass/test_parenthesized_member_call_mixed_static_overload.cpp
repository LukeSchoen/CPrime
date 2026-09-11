// EXPECT_EXIT: 0
/* Parentheses around a member function name keep the whole overload set, so
   the zero-argument call selects the instance member while the one-argument
   call selects the static member of the same name. */
struct A {
  static int f(int value) { return value; }
  int f();
};

int A::f() { return 1; }

int main() {
  A a;
  if ((a.f)() != 1) return 1;
  if ((a.f)(2) != 2) return 2;
  return 0;
}
