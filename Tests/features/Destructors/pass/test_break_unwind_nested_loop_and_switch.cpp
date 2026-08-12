// EXPECT_EXIT: 0
int logv;

struct Item
{
  int v;
  ~Item();
};

Item::~Item()
{
  logv = logv * 10 + this->v;
}

int main(void)
{
  int i;

  for (i = 0; i < 1; ++i)
  {
    Item outer = {1};
    while (1)
    {
      Item inner = {2};
      break;
    }
    break;
  }

  switch (1)
  {
  case 1:
    {
      Item sw = {3};
      break;
    }
  default:
    return 1;
  }

  return logv == 213 ? 0 : 1;
}

