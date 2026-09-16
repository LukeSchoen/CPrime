struct Base { int get() const { return 7; } };
struct Derived : private Base {};
using BasePointer = int (Base::*)() const;
using DerivedPointer = int (Derived::*)() const;
int main() {
  auto converted = reinterpret_cast<DerivedPointer>(&Base::get);
  auto original = reinterpret_cast<BasePointer>(converted);
  Base value;
  return (value.*original)() != 7;
}
