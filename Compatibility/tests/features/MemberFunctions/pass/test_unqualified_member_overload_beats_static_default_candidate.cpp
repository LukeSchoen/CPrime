class Matcher
{
public:
  static int Compare(const char *source, const char *target,
                     int options = 0)
  {
    return 91;
  }

  int Compare(const char *target, int options = 0) const
  {
    return 7;
  }

  int CompareInside(const char *target) const
  {
    return Compare(target, 1);
  }
};

int main()
{
  Matcher matcher;
  return matcher.CompareInside("value") == 7 ? 0 : 1;
}
