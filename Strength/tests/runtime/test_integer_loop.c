// PROFILE_NAME: c.integer.loop
// EXPECT_EXIT: 0

int main(void)
{
  int i;
  int sum;

  sum = 0;
  for (i = 0; i < 20000000; i = i + 1)
    sum = sum + (i & 7);

  return sum == 70000000 ? 0 : 1;
}
