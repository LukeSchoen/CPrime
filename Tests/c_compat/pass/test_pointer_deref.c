// EXPECT_EXIT: 0
// EXPECT_STDOUT:
int main(void)
{
  int v = 9;
  int *p = &v;
  *p = *p + 1;
  return v == 10 ? 0 : 1;
}
