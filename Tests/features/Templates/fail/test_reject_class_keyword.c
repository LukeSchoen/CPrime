// EXPECT_COMPILE_FAIL: 1

template<class T>
T id(T v)
{
  return v;
}

int main(void)
{
  return id(int)(1);
}
