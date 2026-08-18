void consume_int(int value)
{
  (void)value;
}

int main()
{
  int i = 3;
  consume_int(i--);
  return i == 2 ? 0 : 1;
}
