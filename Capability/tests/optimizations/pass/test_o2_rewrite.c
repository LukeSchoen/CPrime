// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -O2

int main(void)
{
  unsigned add = 0;
  unsigned sub = 0;
  unsigned and_value = 0xffffffffu;
  unsigned or_value = 0;
  unsigned xor_value = 0;
  int branch_sum = 0;
  int i;

  for (i = 0; i < 1000; ++i)
  {
    add = add + (unsigned)i;
    sub = sub - (unsigned)i;
    and_value = and_value & ((unsigned)i | 0x100u);
    or_value = or_value | ((unsigned)i & 0x100u);
    xor_value = xor_value ^ (unsigned)i;
  }

  for (i = 0; i < 1000; ++i)
  {
    if (i & 1)
      branch_sum = branch_sum + i;
    else
      branch_sum = branch_sum - 1;
  }

  if (add != 499500u) return 1;
  if (add + sub != 0) return 2;
  if (and_value != 0x100u) return 3;
  if (or_value != 0x100u) return 4;
  if (xor_value != 0) return 5;
  if (branch_sum != 249500) return 6;
  return 0;
}
