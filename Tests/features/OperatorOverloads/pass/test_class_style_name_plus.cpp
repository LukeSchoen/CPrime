// EXPECT_EXIT: 0
// EXPECT_STDOUT:
class clString
{
public:
  int base;
  int operator+(int rhs);
};

int clString::operator+(int rhs)
{
  return this->base + rhs + 100;
}

int main(void)
{
  class clString a = {7};
  return (a + 8) == 115 ? 0 : 1;
}

