// EXPECT_COMPILE_FAIL: 1
struct Num
{
  int value;
  int operator+(int rhs);
};

int Num::operator+(int rhs) { return this->value + rhs; }

int main(void)
{
  struct Num a = {3};
  return a[0];
}

