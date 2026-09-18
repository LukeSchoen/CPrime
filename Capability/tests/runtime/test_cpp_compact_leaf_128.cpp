// PROFILE_NAME: cpp.compact.leaf.128
// EXPECT_EXIT: 0
// -Os cpp_compact cutoff input: the call-free straight-line leaf body is
// 128 bytes before compaction, exactly the cutoff, so -Os compacts it to
// 82 bytes while -O1 keeps the un-compacted 128-byte body.

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
  r = r + 200;

  return r;
}

int main(void)
{
  long long s = 0;
  int i;

  if (leaf(3) != 3 + 242)
    return 2;
  for (i = 0; i < 20000000; i = i + 1)
    s = s + leaf(i & 7);
  return s == 70000000LL + 20000000LL * 242 ? 0 : 1;
}
