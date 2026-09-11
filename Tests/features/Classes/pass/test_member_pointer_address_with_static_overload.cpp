// EXPECT_EXIT: 0
/* The contextual pointer-to-member type selects the instance overload of
   `&A::f`, while the plain function-pointer context selects the static one. */
struct A {
  static int f(int value) { return value; }
  int f();
};

int A::f() { return 1; }

int main() {
  int (A::*member)() = &A::f;
  A a;
  if ((a.*member)() != 1) return 1;

  int (*plain)(int) = &A::f;
  if (plain(2) != 2) return 2;
  return 0;
}
