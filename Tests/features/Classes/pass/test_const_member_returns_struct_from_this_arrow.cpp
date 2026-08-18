struct ReturnStructMember
{
  int value;

  ReturnStructMember Slice(int offset) const;
  ReturnStructMember Left(int count) const;
};

ReturnStructMember ReturnStructMember::Slice(int offset) const
{
  ReturnStructMember out = { value + offset };
  return out;
}

ReturnStructMember ReturnStructMember::Left(int count) const
{
  return this->Slice(count);
}

int main()
{
  ReturnStructMember c = { 3 };
  ReturnStructMember out = c.Left(4);
  return out.value != 7;
}
