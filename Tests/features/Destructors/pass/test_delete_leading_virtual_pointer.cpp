// EXPECT_COMPILE_ARGS: -Werror
int destroyed;
struct Base {
  int value;
  Base(): value(1) {}
  virtual ~Base() { destroyed += value; }
};
struct Other {
  int value;
  Other(): value(2) {}
  virtual ~Other() { destroyed += value; }
};
struct Derived : Base, Other {
  ~Derived() { destroyed += 4; }
};
struct EmptyVirtual {
  virtual ~EmptyVirtual() { destroyed += 8; }
};
int main() {
  for (int i = 0; i < 64; ++i) {
    Base* primary = new Derived;
    delete primary;
    Other* secondary = new Derived;
    delete secondary;
    EmptyVirtual* empty = new EmptyVirtual;
    delete empty;
  }
  Base* null = 0;
  delete null;
  return destroyed != 64 * 22;
}
