struct CounterOwner
{
  int limit;
  int Size() const { return limit; }
};

int main()
{
  CounterOwner owner = { 3 };
  int sum = 0;
  for (int i = 0; i < owner.Size(); i++)
    sum += i;
  return sum == 3 ? 0 : 1;
}
