// PROFILE_NAME: cpp.compact.leaf.125
// EXPECT_EXIT: 0
// -Os cpp_compact cutoff input: the call-free straight-line leaf body is
// 125 bytes before compaction, one statement below the 128-byte cutoff, so
// -Os runs the shared transform set but skips size-oriented compaction.

__attribute__((noinline))
static int leaf(int v)
{
  int r = v;

  r = r + 1;
  r = r + 2;
  r = r + 3;
  r = r + 4;
  r = r + 5;
  r = r + 6;
  r = r + 7;
  r = r + 8;
  r = r + 1;
  r = r + 2;
  r = r + 3;
  r = r + 4;

  return r;
}

int main(void)
{
  long long s = 0;
  int i;

  if (leaf(3) != 3 + 46)
    return 2;
  for (i = 0; i < 20000000; i = i + 1)
    s = s + leaf(i & 7);
  return s == 70000000LL + 20000000LL * 46 ? 0 : 1;
}
