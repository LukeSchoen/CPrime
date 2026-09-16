// EXPECT_EXIT: 0

template<typename T>
T id(T v)
{
  return v;
}

int main(void)
{
  return id(int)(42) == 42 ? 0 : 1;
}
