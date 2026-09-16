int phase, destroyed;
struct Base {
  Base() { if (value() != 1) __builtin_abort(); ++phase; }
  virtual int value() { return 1; }
  virtual ~Base() { ++destroyed; }
};
struct Derived : Base {
  Derived() { if (value() != 2) __builtin_abort(); ++phase; }
  int value() { return 2; }
  ~Derived() { ++destroyed; }
};
int main() {
  Base *value = new Derived;
  if (phase != 2 || value->value() != 2 || dynamic_cast<Derived*>(value) == 0) return 1;
  delete value;
  return destroyed != 2;
}
