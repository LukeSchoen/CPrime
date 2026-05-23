// EXPECT_EXIT: 0
struct Counter
{
  int value;

  Counter()
  {
    this->value = 3;
  }

  Counter(int seed)
  {
    this->value = seed;
  }

  Counter(float seed)
  {
    this->value = (int)(seed * 10.0f);
  }
};

int main(void)
{
  struct Counter a;
  struct Counter b(11);
  float seed = 2.5f;
  struct Counter c(seed);

  if (a.value != 3)
    return 1;
  if (b.value != 11)
    return 2;
  if (c.value != 25)
    return 3;
  return 0;
}
