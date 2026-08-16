void TakeConstRef(const int &value)
{
}

int Value()
{
  return 3;
}

int main()
{
  TakeConstRef(Value() * sizeof(int));
  return 0;
}
