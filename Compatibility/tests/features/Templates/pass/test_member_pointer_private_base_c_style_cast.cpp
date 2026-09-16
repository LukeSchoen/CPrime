struct Base { int member; int get() const { return member; } };
struct Padding { int padding; };
struct Derived : Padding, private Base {
  Derived() : Padding{19}, Base{7} {}
};
using BasePointer = int (Base::*)() const;
using DerivedPointer = int (Derived::*)() const;
int main() {
  DerivedPointer converted = (DerivedPointer)&Base::get;
  BasePointer original = (BasePointer)converted;
  Base value{11};
  Derived derived;
  return (value.*original)() != 11 || (derived.*converted)() != 7;
}
