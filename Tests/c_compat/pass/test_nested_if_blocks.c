// EXPECT_EXIT: 0
// EXPECT_STDOUT:
int main(void)
{
  int x = 1;
  int ok = 0;
  if (x) {
    if (x > 0) {
      ok = 1;
    }
  }
  return ok ? 0 : 1;
}
