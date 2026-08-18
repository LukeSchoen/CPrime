class MemberDefaultArg
{
public:
  void Clear(unsigned int color = 0);
};

void MemberDefaultArg::Clear(unsigned int color)
{
  (void)color;
}

int main()
{
  MemberDefaultArg value;
  value.Clear(0xff009500u);
  value.Clear();
  return 0;
}
