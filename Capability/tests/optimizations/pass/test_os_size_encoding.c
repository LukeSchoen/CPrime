// PROFILE_NAME: c.os.size.encoding
// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Os

static int seed = 1;

static int small_immediates(int value)
{
  int result = value;

  result = result + 7;
  result = result - 9;
  result = result ^ 5;
  result = result & 127;
  result = result | 16;
  result = result * 3;
  if (result < 100)
    result = result + 1000;
  if (result > 1000)
    result = result - 1;
  return result;
}

static int loop_mix(int n)
{
  int i;
  int sum = 0;

  for (i = 0; i < n; ++i)
  {
    if (i & 1)
      sum = sum + 3;
    else
      sum = sum - 4;
  }
  return sum;
}

static int long_branch(int value)
{
  int result = value;

  if (result != 0)
  {
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
    result = result + 1;
  }
  return result;
}

int main(void)
{
  int value = seed;

  if (small_immediates(value) != 366)
    return 1;
  if (loop_mix(value * 1000) != -500)
    return 2;
  if (long_branch(value) != 49)
    return 3;
  return 0;
}
