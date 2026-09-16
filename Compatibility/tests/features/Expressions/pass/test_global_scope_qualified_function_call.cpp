int global_value(int value)
{
  return value + 3;
}

int main()
{
  return ::global_value(4) == 7 ? 0 : 1;
}
