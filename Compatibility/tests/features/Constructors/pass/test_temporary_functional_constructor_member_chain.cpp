struct TemporaryChainString
{
  int value;

  TemporaryChainString(const char *text) : value(text ? 1 : 0) {}

  TemporaryChainString Replace(const char *find, const char *replace) const
  {
    return TemporaryChainString(find && replace ? "x" : 0);
  }

  int Length() const { return value; }
};

int main()
{
  int seen = 0;
  if (TemporaryChainString("abc").Length() > 0)
    seen += 1;
  if (TemporaryChainString("abc").Replace(".", "").Length() > 0)
    seen += 2;
  return seen != 3;
}
