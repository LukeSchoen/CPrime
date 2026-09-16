class Item
{
public:
  int value;
};

class Items
{
public:
  Item data[2];
  int Size() const { return 2; }
  Item &operator[](int index) { return data[index]; }
};

int main()
{
  Items items;
  items.data[0].value = 1;
  items.data[1].value = 2;
  for (Item &item : items) item.value++;
  return items.data[0].value == 2 && items.data[1].value == 3 ? 0 : 1;
}
