// EXPECT_EXIT: 0
int ctor_count;
int dtor_count;

struct Counter
{
  int value;

  Counter()
  {
    this->value = 11;
    ctor_count += 1;
  }

  ~Counter()
  {
    dtor_count += this->value;
  }
};

typedef struct Counter Counter;

int do_work(void)
{
  Counter c;
  if (ctor_count != 1)
    return 1;
  if (c.value != 11)
    return 2;
  return 3;
}

int main(void)
{
  int r = do_work();
  if (r != 3)
    return 4;
  return dtor_count == 11 ? 0 : 5;
}
