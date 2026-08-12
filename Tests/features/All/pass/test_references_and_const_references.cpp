// EXPECT_EXIT: 0

class Counter
{
  int value;

  void add(const int& delta)
  {
    this->value += delta;
  }
};

int read_value(const int& value)
{
  return value;
}

int bump(int& value)
{
  value = value + 1;
  return value;
}

class Holder
{
  int& ref;

  int get()
  {
    return this->ref;
  }
};

int main()
{
  int n = 4;
  int d = 3;
  Counter c;
  c.value = 2;
  c.add(d);
  Holder h = { n };
  if (read_value(n) != 4)
    return 1;
  if (bump(n) != 5)
    return 2;
  if (n != 5)
    return 3;
  if (c.value != 5)
    return 4;
  if (h.get() != 5)
    return 5;
  return 0;
}
