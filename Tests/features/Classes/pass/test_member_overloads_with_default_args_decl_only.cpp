class MemberDefaultsDeclOnly
{
public:
  MemberDefaultsDeclOnly();
  int Find(char target, int startIndex = 0, int mode = 1) const;
  int Find(const MemberDefaultsDeclOnly &target, int startIndex = 0, int mode = 1) const;
  int Last(char target, int startIndex, int mode = 1) const;
  int Last(const MemberDefaultsDeclOnly &target, int startIndex, int mode = 1) const;
};

MemberDefaultsDeclOnly::MemberDefaultsDeclOnly()
{
}

int main()
{
  MemberDefaultsDeclOnly value;
  return 0;
}
