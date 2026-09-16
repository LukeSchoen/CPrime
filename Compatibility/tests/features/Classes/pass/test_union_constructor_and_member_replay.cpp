// EXPECT_EXIT: 0
struct A
{
  char c;
  int i;
};

struct B
{
  char c, d;
};

static A a;

union C
{
  A *p;
  B *q;

  C() : p(&a) {}
  char &foo() { return q->d; }
};

union P
{
  int *p;
  long *q;

  P(int *value) : p(value) {}
  operator int *() { return p; }
};

union O
{
  int a;
  long b;

  O();
};

O::O() { a = 5; }

int main()
{
  C c;
  if (c.p != &a) return 1;
  B b;
  C other;
  other.q = &b;
  other.foo() = 7;
  if (b.d != 7) return 2;
  int value = 3;
  P p(&value);
  if (p != &value) return 3;
  O object;
  return object.a == 5 ? 0 : 4;
}
