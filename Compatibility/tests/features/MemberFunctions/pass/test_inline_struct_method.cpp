// EXPECT_EXIT: 0

struct test
{
  int x;

  int methodFunc(int y)
  {
    return this->x + y;
  }
};

int main(void)
{
  struct test t;
  t.x = 7;
  return t.methodFunc(5) == 12 ? 0 : 1;
}
