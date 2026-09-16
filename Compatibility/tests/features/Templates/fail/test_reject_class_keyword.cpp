// EXPECT_EXIT: 0

template<class T>
T id(T v)
{
  return v;
}

int main(void)
{
  return id(int)(0);
}
