// EXPECT_COMPILE_FAIL: 1

template(T, U)
T fst(T a, U b)
{
  return a;
}

int main(void)
{
  return fst(int)(1, 2);
}
