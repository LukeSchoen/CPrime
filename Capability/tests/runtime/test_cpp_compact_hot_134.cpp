// PROFILE_NAME: cpp.compact.hot.134
// EXPECT_EXIT: 0
// -Os cpp_compact cutoff input: the call-free short hot loop body is
// 134 bytes before compaction, just above the cutoff, so -Os compacts it to
// 120 bytes with one aligned loop-head pad while -O1 keeps the un-compacted
// 134-byte body.  The measured -Os result for this shape is slower than -O1.

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
    s = s + 7;
    s = s + 8;
  }
  return s;
}

int main(void)
{
  long long total = 0;
  int k;

  if (hot(16) != 2 * 28 + 16 * 35)
    return 2;
  for (k = 0; k < 50000; k = k + 1)
    total = total + hot(1000);
  return total == 50000LL * 38500 ? 0 : 1;
}
