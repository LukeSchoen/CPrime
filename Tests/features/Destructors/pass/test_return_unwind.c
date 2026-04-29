// EXPECT_EXIT: 0
int g;

struct Counter
{
  int v;

  ~Counter();
};

Counter::~Counter()
{
  g += this->v;
}

int do_work(int f)
{
  struct Counter c = {5};
  if (f)
    return 7;
  return 0;
}

int main(void)
{
  int r = do_work(1);
  return (r == 7 && g == 5) ? 0 : 1;
}
