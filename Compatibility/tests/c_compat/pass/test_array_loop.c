// EXPECT_EXIT: 0
// EXPECT_STDOUT:
int main(void)
{
  int arr[4] = {1, 2, 3, 4};
  int sum = 0;
  int i;
  for (i = 0; i < 4; ++i)
    sum += arr[i];
  return sum == 10 ? 0 : 1;
}
