// EXPECT_EXIT: 0
int constructed, destroyed;
struct Item {
  int value;
  explicit Item(int n) : value(n) { ++constructed; }
  ~Item() { ++destroyed; }
};
int consume(const Item &item) { return item.value; }
int main() {
  if (consume(static_cast<Item>(17)) != 17) return 1;
  if (constructed != 1 || destroyed != 1) return 2;
  return 0;
}
