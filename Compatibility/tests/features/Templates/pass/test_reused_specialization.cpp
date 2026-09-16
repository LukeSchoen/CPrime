// EXPECT_EXIT: 0

template<typename T>
T add1(T v)
{
  return v + 1;
}

int main(void)
{
  int a = add1(int)(5);
  int b = add1(int)(6);
  return (a == 6 && b == 7) ? 0 : 1;
}
