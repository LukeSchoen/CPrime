static int current;
int copied() { auto value = current; return value; }
struct Range { int values[2]; int *begin() { return values; } int *end() { return values + 2; } };
int ranged(int value)
{
  static Range range;
  range.values[0] = value;
  range.values[1] = value + 1;
  int total = 0;
  for (int item : range) total += item;
  return total;
}
int main()
{
  current = 7;
  if (copied() != 7) return 1;
  current = 19;
  if (copied() != 19) return 2;
  for (int i=0; i<20; ++i) if (ranged(i) != i*2+1) return 3;
  return 0;
}
