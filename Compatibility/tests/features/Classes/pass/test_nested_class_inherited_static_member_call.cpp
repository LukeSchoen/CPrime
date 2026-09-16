// EXPECT_EXIT: 0
// A member body keeps the class scopes of its enclosing classes, so an
// unqualified call names a static member function that the enclosing class
// inherits from a base: from a nested class, from a derived class body, and
// from a constructor that passes `this` to one.
struct Base {
  static int value;
  static int read() { return value; }
  static void write(int next) { value = next; }
};
int Base::value = 3;

struct Owner : private Base {
  struct Nested {
    int peek() { return read(); }
    void bump(int next) { write(next); }
  };
  int peek() { return read(); }
  void bump(int next) { write(next); }
};

template <class T>
struct Wrapper : private Base {
  struct Inner {
    int peek() { return read(); }
    void bump(int next) { write(next); }
  };
  int peek() { return read(); }
  void bump(int next) { write(next); }
};

struct Probe : private Base {
  Probe() { write(via(this)); }
  int peek() { return read(); }
  static int via(Probe *object) { return object->peek(); }
};

int main() {
  Owner::Nested nested;
  if (nested.peek() != 3) return 1;
  nested.bump(7);
  if (Base::read() != 7) return 2;
  Owner owner;
  owner.bump(11);
  if (Base::read() != 11) return 3;
  Wrapper<int> wrapper;
  wrapper.bump(13);
  if (Base::read() != 13) return 4;
  Wrapper<int>::Inner inner;
  inner.bump(17);
  if (Base::read() != 17) return 5;
  Probe probe;
  return Base::read() != 17;
}
