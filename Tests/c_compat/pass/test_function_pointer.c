// EXPECT_EXIT: 0
// EXPECT_STDOUT:
int square(int x)
{
  return x * x;
}

int main(void)
{
  int (*fn)(int) = square;
  return fn(6) == 36 ? 0 : 1;
}
