// EXPECT_EXIT: 0

int main()
{
  int i = int(5);
  float f = float(2);
  double d = double(3);
  return i == 5 && f == 2.0f && d == 3.0 ? 0 : 1;
}
