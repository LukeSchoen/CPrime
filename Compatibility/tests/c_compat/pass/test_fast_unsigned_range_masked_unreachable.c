// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -O2

static unsigned char source;
void unreachable_range_bit(void);

static int keeps_reachable_bits(unsigned char value)
{
  int copy;
  source = value;
  copy = source;
  if (copy < 4 && (source & 2)) return 1;
  return 0;
}

static int invalidates_after_store(unsigned char value)
{
  int copy;
  source = value;
  copy = source;
  if (copy < 4) {
    source = 4;
    if (source & 4) return 1;
  }
  return 0;
}

int main(void)
{
  int copy;
  source = 0;
  copy = source;
  if (copy < 4) {
    if (source & 4) unreachable_range_bit();
  }
  return keeps_reachable_bits(2) != 1 || keeps_reachable_bits(4) != 0
      || invalidates_after_store(0) != 1;
}
