// EXPECT_EXIT: 0

template<typename T>
T id(T v)
{
  return v;
}

int main(void)
{
  int i = 11;
  float f = 2.5f;

  if (id(i) != 11)
    return 1;
  if (id(f) != 2.5f)
    return 2;

  return 0;
}
