// EXPECT_EXIT: 0
// EXPECT_STDOUT:
class Num
{
public:
  int value;

  int operator+(int rhs)
  {
    return this->value + rhs + 3;
  }
};

int main(void)
{
  class Num a = {4};
  return (a + 5) == 12 ? 0 : 1;
}
