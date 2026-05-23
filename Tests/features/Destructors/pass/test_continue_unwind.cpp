// EXPECT_EXIT: 0
int g;

struct Item
{
  int v;

  ~Item();
};

Item::~Item()
{
  g += this->v;
}

int main(void)
{
  int i;
  for (i = 0; i < 3; ++i)
  {
    struct Item it = {1};
    if (i < 2)
      continue;
  }
  return g == 3 ? 0 : 1;
}
