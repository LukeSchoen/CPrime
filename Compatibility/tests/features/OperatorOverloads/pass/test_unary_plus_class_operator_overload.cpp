// A class operand of prefix '+' must resolve the overloaded operator+.
// The builtin path only folded scalar and pointer operands, so a class
// prvalue reached "invalid operand types for binary operation" instead of the
// free operator.  The non-const reference overload below cannot bind the
// prvalue, so the const reference overload has to win.

struct A {
};

int picked;

void operator+(A &) { picked = 1; }
void operator+(const A &) { picked = 2; }

struct B {
  int operator+() const { return 3; }
};

int main() {
  +A();
  if (picked != 2) return 1;

  if (+B() != 3) return 2;
  return 0;
}
