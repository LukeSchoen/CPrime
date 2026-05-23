// EXPECT_COMPILE_FAIL: 1

template(T)
T twice(T v)
{
  return v + v;
}

int main(void)
{
  return twice(7);
}
