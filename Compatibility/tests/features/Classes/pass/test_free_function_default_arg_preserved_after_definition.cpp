int AddDefault(int value, int amount = 3);

int AddDefault(int value, int amount)
{
  return value + amount;
}

int main()
{
  return AddDefault(4) == 7 ? 0 : 1;
}
