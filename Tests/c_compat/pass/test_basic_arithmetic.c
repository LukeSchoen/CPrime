// EXPECT_EXIT: 0
// EXPECT_STDOUT:
int main(void)
{
  int a = 2;
  int b = 3;
  return (a + b == 5) ? 0 : 1;
}
