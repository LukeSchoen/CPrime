struct Pool
{
  static int Current();
};

int Pool::Current()
{
  return 1;
}

struct Owner
{
  struct Item
  {
    int value;
  };

  struct Items
  {
    Item values[1];
    int Size() const { return 1; }
    Item &operator[](int i) { return values[i]; }
  };

  Items items;
  int round;

  void Render();
};

void Owner::Render()
{
  if (round != Pool::Current())
    round = Pool::Current();

  for (Item &item : items)
    item.value = item.value + 1;
}

int main()
{
  Owner owner;
  owner.round = 0;
  owner.items.values[0].value = 1;
  return 0;
}
