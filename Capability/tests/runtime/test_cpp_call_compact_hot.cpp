// PROFILE_NAME: cpp.call.compact.hot
// EXPECT_EXIT: 0
// -O2/-Os C++ call-containing frame: the hot function contains both the loop
// state and a call, so it is visible to the bounded fast-frame pass instead of
// only compacting the caller.

__attribute__((noinline))
static int mix(int x)
{
  int r = x;

  r = r + 1;
  r = r + 2;
  r = r + 3;
  r = r + 4;
  r = r + 5;
  r = r + 6;
  r = r + 7;
  r = r + 8;
  return r;
}

__attribute__((noinline))
static int hot(int n)
{
  int i = 0;
  int s = 0;

  while (i < n)
  {
    s = s + mix(i & 7);
    s = s + 2;
    s = s + 3;
    s = s + 4;
    s = s + 5;
    s = s + 6;
    s = s + 7;
    i = i + 1;
  }
  return s;
}

int main(void)
{
  long long total = 0;
  int expect = 0;
  int k;

  for (k = 0; k < 16; k = k + 1)
    expect = expect + (k & 7) + 63;
  if (hot(16) != expect)
    return 2;
  for (k = 0; k < 50000; k = k + 1)
    total = total + hot(1000);
  return total == 50000LL * 66500 ? 0 : 1;
}
