struct Base {
  virtual operator int() const { return 3; }
};
typedef int Result;
struct Derived : Base {
  operator Result() const { return 7; }
};
struct Explicit {
  explicit operator int() const { return 11; }
};
typedef int Integer;
int main() {
  Derived value;
  const Base &base = value;
  return int(base) != 7 || Integer(base) != 7 || int(Explicit()) != 11;
}
