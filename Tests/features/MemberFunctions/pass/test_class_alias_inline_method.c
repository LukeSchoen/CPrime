// EXPECT_EXIT: 0

class test_class
{
  int x;

  int methodFunc(int y)
  {
    return this->x + y;
  }
};

int main(void)
{
  class test_class t;
  t.x = 9;
  return t.methodFunc(2) == 11 ? 0 : 1;
}
