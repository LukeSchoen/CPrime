class TempSlotVirtual
{
public:
  TempSlotVirtual(int initial) : value(initial) {}

  virtual int Read() const
  {
    return value;
  }

private:
  int value;
  char padding[320];
};

int ReadFromBranch(bool takeEarlyBranch, TempSlotVirtual* external)
{
  if (takeEarlyBranch)
  {
    return external->Read();
  }

  TempSlotVirtual local(42);
  int first = local.Read();
  int second = local.Read();
  return first == 42 && second == 42 ? 0 : 1;
}

int main()
{
  TempSlotVirtual external(7);
  return ReadFromBranch(false, &external);
}
