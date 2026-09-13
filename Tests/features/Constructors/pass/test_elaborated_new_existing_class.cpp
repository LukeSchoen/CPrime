struct Item {
  int value;
  Item() : value(17) {}
};

int main() {
  Item *plain = new Item;
  class Item *elaborated = new class Item;
  return plain->value != 17 || elaborated->value != 17;
}
