bool copied;

struct Base {
  Base() {}
  Base(const Base &) { copied = true; }
};

struct Derived : Base {
  Derived() {}
  Derived(const Derived &) : Base() {}
};

Derived value;
Base base;

Derived make() {
  return value;
}

int main() {
  base = (true ? make() : base);
  return !copied;
}
