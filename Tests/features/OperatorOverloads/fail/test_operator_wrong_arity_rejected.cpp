// EXPECT_COMPILE_FAIL: 1
struct Num
{
  int value;

  int operator+(int lhs, int rhs);
};

int Num::operator+(int lhs, int rhs)
{
  return this->value + lhs + rhs;
}

int main(void)
{
  struct Num a = {1};
  return a + 2;
}
