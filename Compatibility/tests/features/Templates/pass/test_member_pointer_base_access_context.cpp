struct Base { constexpr int get() const { return 7; } };
struct Private : private Base {
  int read() const {
    int (Private::*pointer)() const = &Base::get;
    return (this->*pointer)();
  }
  struct Nested {
    static int read(const Private &value) {
      auto pointer = &Base::get;
      return (value.*pointer)();
    }
  };
  friend int read_friend(const Private &);
};
int read_friend(const Private &value) {
  auto base_pointer = &Base::get;
  int (Private::*pointer)() const = base_pointer;
  return (value.*pointer)();
}
struct Protected : protected Base {};
struct Child : Protected {
  int read() const { auto pointer = &Base::get; return (this->*pointer)(); }
};
int main() {
  Private value;
  Child child;
  return value.read() != 7 || Private::Nested::read(value) != 7
      || read_friend(value) != 7 || child.read() != 7;
}
