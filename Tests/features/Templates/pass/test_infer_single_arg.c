// EXPECT_EXIT: 0

template<typename T>
T id(T v)
{
  return v;
}

int main(void)
{
  int x = 7;
  return id(42) == 42 && id(x) == 7 ? 0 : 1;
}
