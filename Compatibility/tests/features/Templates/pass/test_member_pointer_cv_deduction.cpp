struct Item { int value; };
struct Derived : Item {};
int Item::* original = &Item::value;
const volatile int Item::* qualified = &Item::value;
int Derived::* inherited = original;
typedef int Item::* Member;
Member empty = Member(0);
template<class T> int read(Item &item, const T Item::* member) {
  return item.*member;
}
struct Pointer {
  Item *value;
  operator Item*() { return value; }
};
int main() {
  Derived item;
  item.value = 37;
  Pointer pointer = { &item };
  if (read(item, original) != 37 || item.*qualified != 37) return 1;
  if (item.*inherited != 37 || pointer->*original != 37) return 2;
  Member local = 0;
  local = &Item::value;
  if (local == empty || item.*local != 37) return 3;
  return 0;
}
