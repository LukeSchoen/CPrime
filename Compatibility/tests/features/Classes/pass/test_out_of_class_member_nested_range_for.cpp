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
  // `indexes` is a member read from a const object, so the range is
  // `const Items` and the range-for element is the const overload's
  // `const Item &`.  A non-const loop variable cannot bind to it.
  for (const Item &item : indexes)
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
  return owner.Sum() == 12 ? 0 : 1;
}
