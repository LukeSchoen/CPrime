// PROFILE_NAME: cpp.os.size.encoding
// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Os

template<typename T>
static T small_immediates(T value)
{
  T result = value;

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
    result = result + 1;
  }
  return result;
}

class LoopMix
{
  int step;

public:
  explicit LoopMix(int value) : step(value) {}

  int mix(int n) const
  {
    int i;
    int sum = 0;

    for (i = 0; i < n; ++i)
      sum = sum + (i & 1 ? step : -step);
    return sum;
  }
};

int main(void)
{
  int value = 1;

  if (small_immediates<int>(value) != 366)
    return 1;
  if (long_branch(value) != 50)
    return 2;
  if (LoopMix(5).mix(1000) != 0)
    return 3;
  return 0;
}
