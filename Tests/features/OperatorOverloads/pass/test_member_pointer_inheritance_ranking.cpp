// EXPECT_EXIT: 0
struct Base { int value; int read() { return value; } };
struct Prefix { int padding; };
struct Middle : Prefix, Base {};
struct Leaf : Middle {};
int select(int Middle::* member) {
  Middle object;
  object.value = 7;
  return object.*member;
}
int select(int Leaf::*) { return 99; }
int call(int (Leaf::*)()) { return 99; }
int call(int (Middle::* member)()) {
  Middle object;
  object.value = 11;
  return (object.*member)();
}
int main() {
  int Base::* data = &Base::value;
  int (Base::* function)() = &Base::read;
  if (select(data) != 7) return 1;
  if (call(function) != 11) return 2;
  return 0;
}
