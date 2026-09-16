int AddDefaultPrototype(int value, int amount = 3);

int main()
{
  return AddDefaultPrototype(4) == 7 ? 0 : 1;
}

int AddDefaultPrototype(int value, int amount)
{
  return value + amount;
}
