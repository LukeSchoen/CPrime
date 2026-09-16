// EXPECT_EXIT: 0

template<typename T>
T max2(T a, T b)
{
  return a > b ? a : b;
}

int main(void)
{
  int a = max2(int)(3, 7);
  long b = max2(long)(10L, 4L);
  return (a == 7 && b == 10L) ? 0 : 1;
}
