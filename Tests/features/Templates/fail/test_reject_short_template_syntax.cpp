// EXPECT_COMPILE_FAIL: 1

template(T)
T id(T v)
{
  return v;
}

int main(void)
{
  return id(int)(1);
}
