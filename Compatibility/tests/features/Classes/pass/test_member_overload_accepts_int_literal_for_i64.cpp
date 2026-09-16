typedef signed long long int i64;

struct I64MemberArg
{
  i64 Pick(i64 start, i64 count) const;
  i64 Left(i64 count) const;
};

i64 I64MemberArg::Pick(i64 start, i64 count) const
{
  return start + count;
}

i64 I64MemberArg::Left(i64 count) const
{
  return this->Pick(0, count);
}

int main()
{
  I64MemberArg value;
  return value.Left(4) != 4;
}
