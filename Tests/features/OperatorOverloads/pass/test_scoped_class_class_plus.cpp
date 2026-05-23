// EXPECT_EXIT: 0
// EXPECT_STDOUT:
struct Num
{
  int value;

  struct Num operator+(struct Num rhs);
};

struct Num Num::operator+(struct Num rhs)
{
  struct Num out = {this->value + rhs.value};
  return out;
}

int main(void)
{
  struct Num a = {4};
  struct Num b = {8};
  struct Num c = a + b;
  return c.value == 12 ? 0 : 1;
}
