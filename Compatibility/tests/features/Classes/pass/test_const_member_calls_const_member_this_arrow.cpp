struct ConstMemberCall
{
  int value;

  int Get(int offset) const;
  int Left(int count) const;
};

int ConstMemberCall::Get(int offset) const
{
  return value + offset;
}

int ConstMemberCall::Left(int count) const
{
  return this->Get(count);
}

int main()
{
  ConstMemberCall c = { 3 };
  return c.Left(4) != 7;
}
