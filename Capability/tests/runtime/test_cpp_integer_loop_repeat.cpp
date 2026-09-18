// PROFILE_NAME: cpp.integer.loop.repeat
// EXPECT_EXIT: 0

int main(void)
{
  int pass;
  int i;
  int sum = 0;

  for (pass = 0; pass < 10; pass = pass + 1)
    for (i = 0; i < 20000000; i = i + 1)
      sum = sum + (i & 7);
  return sum == 700000000 ? 0 : 1;
}
