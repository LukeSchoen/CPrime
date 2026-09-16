// EXPECT_EXIT: 0

struct first
{
  int x;

  int methodFunc(int y)
  {
    return this->x + y;
  }
};

struct second
{
  int x;

  int methodFunc(int y)
  {
    return this->x - y;
  }
};

int main(void)
{
  struct first a;
  struct second b;
  a.x = 10;
  b.x = 10;
  if (a.methodFunc(3) != 13)
    return 1;
  return b.methodFunc(3) == 7 ? 0 : 2;
}
