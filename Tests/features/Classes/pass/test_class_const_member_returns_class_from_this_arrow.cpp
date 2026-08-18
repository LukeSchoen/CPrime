class ClassReturnMember
{
private:
  int value;

public:
  ClassReturnMember();
  ClassReturnMember Slice(int offset) const;
  ClassReturnMember Left(int count) const;
};

ClassReturnMember::ClassReturnMember() : value(3) {}

ClassReturnMember ClassReturnMember::Slice(int offset) const
{
  ClassReturnMember out;
  out.value = value + offset;
  return out;
}

ClassReturnMember ClassReturnMember::Left(int count) const
{
  return this->Slice(count);
}

int main()
{
  ClassReturnMember c;
  ClassReturnMember out = c.Left(4);
  (void)out;
  return 0;
}
