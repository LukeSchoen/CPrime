struct Item
{
  int value;
};

struct Items
{
  Item data[1];
  int Size() const { return 1; }
  Item& operator[](int index) { return data[index]; }
};

int main()
{
  Items items;
  items.data[0].value = 1;
  for (Item &attribute : items)
    attribute.value = 2;
  return items.data[0].value == 2 ? 0 : 1;
}
