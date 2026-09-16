// EXPECT_EXIT: 0
struct A {};
struct B : A {};
struct C : B {};

int select(A &) { return 1; }
int select(B &) { return 2; }

struct Convertible {
  C value;
  operator C &() { return value; }
};

int main()
{
  C value;
  Convertible converted;
  return select(value) == 2 && select(converted) == 2 ? 0 : 1;
}
