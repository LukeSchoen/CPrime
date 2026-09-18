// EXPECT_EXIT: 0
// EXPECT_STDOUT:
class TextBlock
{
public:
  int base;
  int operator+(int rhs);
};

int TextBlock::operator+(int rhs)
{
  return this->base + rhs + 100;
}

int main(void)
{
  class TextBlock a = {7};
  return (a + 8) == 115 ? 0 : 1;
}

