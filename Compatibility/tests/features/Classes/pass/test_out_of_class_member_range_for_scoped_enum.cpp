enum Kind : int
{
  Kind_A = 1,
};

struct Owner
{
  struct Item
  {
    Kind kind;
  };

  struct Items
  {
    Item values[1];
    int Size() const { return 1; }
    Item &operator[](int i) { return values[i]; }
    const Item &operator[](int i) const { return values[i]; }
  };

  Items items;

  int Count() const;
};

int Owner::Count() const
{
  int count = 0;
  for (const Item &item : items)
  {
    if (item.kind == Kind::Kind_A)
      count = count + 1;
  }
  return count;
}

int main()
{
  Owner owner;
  owner.items.values[0].kind = Kind_A;
  owner.Count();
  return 0;
}
