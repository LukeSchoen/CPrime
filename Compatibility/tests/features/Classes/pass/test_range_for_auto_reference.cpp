struct Item
{
  int value;
};

struct Items
{
  Item data[2];

  Item *begin() { return data; }
  Item *end() { return data + 2; }
  int Size() { return 2; }
  Item &operator[](int index) { return data[index]; }
};

int main()
{
  Items items;
  items.data[0].value = 1;
  items.data[1].value = 2;
  int sum = 0;
  for (auto &piece : items)
    sum += piece.value;
  return sum != 3;
}
