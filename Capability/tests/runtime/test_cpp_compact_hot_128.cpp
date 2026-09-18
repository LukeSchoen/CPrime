// PROFILE_NAME: cpp.compact.hot.128
// EXPECT_EXIT: 0
// -Os cpp_compact cutoff input: the call-free short hot loop body is
// 128 bytes before compaction, exactly the cutoff, so -Os compacts it to
// 117 bytes with one aligned loop-head pad while -O1 keeps the un-compacted
// 128-byte body.

__attribute__((noinline))
static int hot(int n)
{
  int i;
  int s = 0;

  for (i = 0; i < n; i = i + 1)
  {
    s = s + (i & 7);
    s = s + 2;
    s = s + 3;
    s = s + 4;
    s = s + 5;
    s = s + 6;
    s = s + 200;
  }
  return s;
}

int main(void)
{
  long long total = 0;
  int k;

  if (hot(16) != 2 * 28 + 16 * 220)
    return 2;
  for (k = 0; k < 50000; k = k + 1)
    total = total + hot(1000);
  return total == 50000LL * 223500 ? 0 : 1;
}
