// EXPECT_COMPILE_FAIL: 1
struct Num
{
  int value;

  int operator+(int rhs);
};

void Num::operator+(int rhs)
{
  this->value += rhs;
}

int main(void)
{
  struct Num a = {1};
  return a + 2;
}
