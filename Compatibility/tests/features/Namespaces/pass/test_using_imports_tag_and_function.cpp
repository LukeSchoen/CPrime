// EXPECT_EXIT: 0
namespace Source {
  enum Value { selected = 3 };
  int Value(int number) { return number + 2; }
  struct Item { int number; };
  int Item = 7;
}
int main() {
  using Source::Value;
  enum Value value = Source::selected;
  using Source::Item;
  struct Item item;
  item.number = Item;
  return Value(value) != 5 || sizeof(value) != sizeof(enum Source::Value)
      || item.number != 7;
}
