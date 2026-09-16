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

struct Store
{
  Items items;
  void Clear();
  int Count() const;
};

void Store::Clear()
{
  for (Item &item : items)
    item.value = 3;
}

int Store::Count() const
{
  return items.data[0].value;
}

int main()
{
  Store store;
  store.items.data[0].value = 1;
  store.Clear();
  return store.Count() == 3 ? 0 : 1;
}
