struct Owner
{
  struct Item
  {
    int value;
  };

  struct Items
  {
    Item values[3];
    int Size() const { return 3; }
    Item &operator[](int i) { return values[i]; }
    const Item &operator[](int i) const { return values[i]; }
  };

  Items indexes;
  int bias;

  int Sum() const;
};

int Owner::Sum() const
{
  int total = 0;
  for (Item &item : indexes)
    total = total + item.value + bias;
  return total;
}

int main()
{
  Owner owner;
  owner.bias = 1;
  owner.indexes.values[0].value = 2;
  owner.indexes.values[1].value = 3;
  owner.indexes.values[2].value = 4;
  owner.Sum();
  return 0;
}
