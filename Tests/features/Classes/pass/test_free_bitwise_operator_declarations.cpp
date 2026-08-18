enum Flags
{
  FlagA = 1,
  FlagB = 2
};

Flags operator |(const Flags &a, const Flags &b);
Flags operator &(const Flags &a, const Flags &b);
Flags operator ^(const Flags &a, const Flags &b);

int main()
{
  return 0;
}
