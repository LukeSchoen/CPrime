struct LabelMemberCall
{
private:
  int hidden;

public:
  int Get(int offset) const;
  int Left(int count) const;
};

int LabelMemberCall::Get(int offset) const
{
  return hidden + offset;
}

int LabelMemberCall::Left(int count) const
{
  return this->Get(count);
}

int main()
{
  LabelMemberCall c = { 3 };
  return c.Left(4) != 7;
}
