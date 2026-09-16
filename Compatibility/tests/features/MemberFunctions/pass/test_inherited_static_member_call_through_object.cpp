// EXPECT_EXIT: 0
/* A diamond that inherits one static member through several subobjects is not
   ambiguous, so calling it through the derived object still resolves. */
struct A {
  static int f() { return 7; }
};
struct B : public A { };
struct C : public A { };
struct D : public B, public C { };

int main() {
  D d;
  if (d.f() != 7) return 1;
  return 0;
}
