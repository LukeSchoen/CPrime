struct Number {
  int value;
  Number(int v) : value(v) {}
  Number operator+(const Number& rhs) const { return Number(value + rhs.value); }
  static int seed() { return 6; }
};
struct Holder { int value; Holder(const Number& n) : value(n.value) {} };
int main() {
  int start = 3;
  Holder a(Number(start) + Number(4));
  Holder b(Number{5});
  Number c(Number::seed());
  Holder declaration(Number(named));
  return a.value != 7 || b.value != 5 || c.value != 6;
}
