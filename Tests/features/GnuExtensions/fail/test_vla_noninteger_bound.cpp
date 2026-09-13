// EXPECT_COMPILE_FAIL: 1

int main()
{
  int count = 2;
  char *invalid = 0;
  int (*matrix)[count][invalid];
  (void)matrix;
  return 0;
}
