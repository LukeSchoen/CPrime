class SelfReturnOverload
{
public:
  SelfReturnOverload();
  SelfReturnOverload Slice(int start, int count) const;
  SelfReturnOverload Slice(int start) const;
  int value;
};

SelfReturnOverload::SelfReturnOverload() : value(0)
{
}

SelfReturnOverload SelfReturnOverload::Slice(int start, int count) const
{
  SelfReturnOverload result;
  result.value = start + count;
  return result;
}

SelfReturnOverload SelfReturnOverload::Slice(int start) const
{
  return Slice(start, 2);
}

int main()
{
  SelfReturnOverload value;
  return value.Slice(5).value == 7 ? 0 : 1;
}
