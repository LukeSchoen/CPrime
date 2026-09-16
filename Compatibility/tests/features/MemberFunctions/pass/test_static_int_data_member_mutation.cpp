class Counter {
  static int value;
};

int Counter::value;

int main(void)
{
  Counter::value = 11;
  Counter::value = Counter::value + 3;
  return Counter::value - 14;
}
