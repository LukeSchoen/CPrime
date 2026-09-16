// EXPECT_EXIT: 0
/* The contextual pointer-to-member type selects the instance overload of
   `&A::f`, while the plain function-pointer context selects the static one. */
struct A {
  static int f(int value) { return value; }
  int f();
};

int A::f() { return 1; }

struct Converter {
  static int apply(int value) { return value + 1; }
  static int apply(const char *) { return 20; }
};

int main() {
  int (A::*member)() = &A::f;
  A a;
  if ((a.*member)() != 1) return 1;

  int (*plain)(int) = &A::f;
  if (plain(2) != 2) return 2;
  if ((a.f)() != 1 || (a.f)(2) != 2) return 3;
  Converter converter;
  if (converter.apply(6) != 7) return 4;
  return 0;
}
