// EXPECT_EXIT: 0
int g;

struct Guard
{
  int value;

  ~Guard();
};

Guard::~Guard()
{
  g += this->value;
}

int main(void)
{
  int i;
  struct Guard outer = {100};

  for (i = 0; i < 3; ++i)
  {
    struct Guard inner = {1};
    if (i < 2)
      continue;
    g += 10;
  }

  if (g != 13)
    return 1;

  return 0;
}
