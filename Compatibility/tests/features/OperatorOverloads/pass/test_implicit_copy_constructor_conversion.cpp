struct Item { int value; };

struct Source {
  Item value;
  Source() { value.value = 29; }
  operator const Item &() const { return value; }
};

int main() {
  Item *result = new Item(Source());
  return result->value != 29;
}
