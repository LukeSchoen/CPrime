// EXPECT_EXIT: 0
// EXPECT_STDOUT:
struct Num
{
  int value;

  int operator%(int rhs);
  int operator==(int rhs);
  int operator!=(int rhs);
  int operator<(int rhs);
  int operator<=(int rhs);
  int operator>(int rhs);
  int operator>=(int rhs);
  int operator+=(int rhs);
  int operator[](int idx);
  int operator-();
};

int Num::operator%(int rhs) { return this->value % rhs; }
int Num::operator==(int rhs) { return this->value == rhs; }
int Num::operator!=(int rhs) { return this->value != rhs; }
int Num::operator<(int rhs) { return this->value < rhs; }
int Num::operator<=(int rhs) { return this->value <= rhs; }
int Num::operator>(int rhs) { return this->value > rhs; }
int Num::operator>=(int rhs) { return this->value >= rhs; }
int Num::operator+=(int rhs) { this->value += rhs; return this->value; }
int Num::operator[](int idx) { return this->value + idx; }
int Num::operator-() { return -this->value; }

int main(void)
{
  struct Num a = {20};
  int ok = 1;

  if ((a % 6) != 2) ok = 0;
  if ((a == 20) != 1) ok = 0;
  if ((a != 20) != 0) ok = 0;
  if ((a < 21) != 1) ok = 0;
  if ((a <= 20) != 1) ok = 0;
  if ((a > 19) != 1) ok = 0;
  if ((a >= 20) != 1) ok = 0;
  if ((-a) != -20) ok = 0;
  if ((a[3]) != 23) ok = 0;
  if ((a += 5) != 25) ok = 0;

  return ok ? 0 : 1;
}
