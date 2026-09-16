// EXPECT_EXIT: 0
struct Accumulator
{
  int value;

  void set()
  {
    this->value = 5;
  }

  void set(int value)
  {
    this->value = value;
  }

  void set(float value)
  {
    this->value = (int)(value * 10.0f);
  }

  void set(int a, int b)
  {
    this->value = a + b;
  }
};

int main(void)
{
  struct Accumulator acc;

  acc.set();
  if (acc.value != 5)
    return 1;
  acc.set(8);
  if (acc.value != 8)
    return 2;
  {
    float value = 1.5f;
    acc.set(value);
  }
  if (acc.value != 15)
    return 3;
  acc.set(9, 4);
  if (acc.value != 13)
    return 4;
  return 0;
}
