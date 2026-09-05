struct Padding { int padding; };
struct Base { int value; int add(int n) { return value + n; } };
struct Derived : Padding, Base {};
int main() {
  int Base::*empty = nullptr;
  int Base::*base_member = &Base::value;
  if (empty || !base_member || empty != nullptr || base_member == nullptr)
    return 1;
  int Derived::*member = base_member;
  int Derived::*null_member = empty;
  if (null_member || member != base_member) return 2;
  Derived derived;
  derived.padding = 3;
  derived.value = 7;
  derived.*member = 11;
  if (derived.value != 11 || derived.padding != 3) return 3;
  int (Derived::*method)(int) = &Base::add;
  if ((derived.*method)(4) != 15) return 4;
  int Base::*back = static_cast<int Base::*>(member);
  if (back != base_member) return 5;
  return 0;
}
