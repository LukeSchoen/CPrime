class InlineSelfOverload
{
public:
  InlineSelfOverload();
  InlineSelfOverload &operator=(const InlineSelfOverload &other);
  InlineSelfOverload Slice(int start, int count) const;
  InlineSelfOverload Slice(int start) const;

  bool Drop()
  {
    *this = Slice(1);
    return true;
  }

  int value;
};

InlineSelfOverload::InlineSelfOverload() : value(0)
{
}

InlineSelfOverload &InlineSelfOverload::operator=(const InlineSelfOverload &other)
{
  value = other.value;
  return *this;
}

InlineSelfOverload InlineSelfOverload::Slice(int start, int count) const
{
  InlineSelfOverload result;
  result.value = start + count;
  return result;
}

InlineSelfOverload InlineSelfOverload::Slice(int start) const
{
  return Slice(start, 2);
}

int main()
{
  InlineSelfOverload value;
  value.Drop();
  return value.value == 3 ? 0 : 1;
}
