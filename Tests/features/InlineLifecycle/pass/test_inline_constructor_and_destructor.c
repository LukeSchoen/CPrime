// EXPECT_EXIT: 0
int ctor_count;
int dtor_count;

struct Box
{
  int value;

  Box()
  {
    this->value = 7;
    ctor_count += 1;
  }

  ~Box()
  {
    dtor_count += this->value;
  }
};

int do_work(void)
{
  struct Box b;
  if (ctor_count != 1)
    return 1;
  if (b.value != 7)
    return 2;
  return 3;
}

int main(void)
{
  int r = do_work();
  if (r != 3)
    return 4;
  if (dtor_count != 7)
    return 5;
  return 0;
}
