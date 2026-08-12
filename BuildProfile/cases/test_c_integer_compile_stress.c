// PROFILE_NAME: c.integer.compile.stress

int mix(int seed)
{
  int acc;
  int i;
  acc = seed;
  for (i = 0; i < 4096; i = i + 1) {
    acc = acc + (i * 3);
    acc = acc ^ (acc >> 5);
    acc = acc + (acc << 2);
    if ((acc & 7) == 3)
      acc = acc - i;
    else
      acc = acc + seed;
  }
  return acc;
}

int main(void)
{
  int total;
  int i;
  total = 0;
  for (i = 0; i < 256; i = i + 1)
    total = total + mix(i);
  return total == 0 ? 1 : 0;
}
