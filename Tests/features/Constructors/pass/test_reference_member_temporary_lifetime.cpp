// EXPECT_EXIT: 0
int live, body_ready, early_destroy;

struct Item
{
  Item() { ++live; }
  ~Item()
  {
    --live;
    if (!body_ready) ++early_destroy;
  }
};

struct Owner
{
  const Item &first;
  const Item &second;

  Owner() : first(Item()), second(Item())
  {
    if (live != 2) early_destroy = 1;
    body_ready = 1;
  }
};

int main()
{
  Owner owner;
  return early_destroy || live != 0;
}
