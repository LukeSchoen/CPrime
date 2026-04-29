// EXPECT_EXIT: 0

struct holder
{
  int x;
  int (*fn)(int);

  int methodFunc(int y)
  {
    return this->x + y;
  }
};

int plus_one(int v)
{
  return v + 1;
}

int main(void)
{
  struct holder h;
  h.x = 3;
  h.fn = plus_one;
  if (h.fn(8) != 9)
    return 1;
  return h.methodFunc(4) == 7 ? 0 : 2;
}
