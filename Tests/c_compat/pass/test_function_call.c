// EXPECT_EXIT: 0
// EXPECT_STDOUT:
int add(int a, int b)
{
  return a + b;
}

int main(void)
{
  return add(20, 22) == 42 ? 0 : 1;
}
