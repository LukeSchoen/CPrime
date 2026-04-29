// EXPECT_EXIT: 0
// EXPECT_STDOUT:
struct Num
{
  int value;

  int operator+(int rhs);
  int operator-(int rhs);
  int operator*(int rhs);
  int operator/(int rhs);
  int operator<<(int rhs);
  int operator>>(int rhs);
};

int Num::operator+(int rhs) { return this->value + rhs; }
int Num::operator-(int rhs) { return this->value - rhs; }
int Num::operator*(int rhs) { return this->value * rhs; }
int Num::operator/(int rhs) { return this->value / rhs; }
int Num::operator<<(int rhs) { return this->value << rhs; }
int Num::operator>>(int rhs) { return this->value >> rhs; }

int main(void)
{
  struct Num a = {20};
  int ok = 1;

  if ((a + 5) != 25) ok = 0;
  if ((a - 5) != 15) ok = 0;
  if ((a * 5) != 100) ok = 0;
  if ((a / 5) != 4) ok = 0;
  if ((a << 1) != 40) ok = 0;
  if ((a >> 1) != 10) ok = 0;

  return ok ? 0 : 1;
}
