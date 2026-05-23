class Counter {
  static int value;
};

int Counter::value = 7;

int main(void)
{
  return Counter::value - 7;
}
