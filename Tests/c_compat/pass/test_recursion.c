// EXPECT_EXIT: 0
// EXPECT_STDOUT:
int fact(int n)
{
  if (n <= 1)
    return 1;
  return n * fact(n - 1);
}

int main(void)
{
  return fact(5) == 120 ? 0 : 1;
}
