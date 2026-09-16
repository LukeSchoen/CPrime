typedef signed long long i64;

struct SmallString
{
  SmallString(const char *text) : value(text) {}
  const char *value;
};

struct ScanLike
{
  static i64 Int(const char *text, int *pLength = 0);
  static i64 Int(const SmallString &text);
};

i64 ScanLike::Int(const char *text, int *pLength)
{
  return text[0] == '7' ? 7 : 0;
}

i64 ScanLike::Int(const SmallString &text)
{
  return 99;
}

double ReadExponent(const char *text, int offset)
{
  return (double)ScanLike::Int(text + offset + 1);
}

int main()
{
  return ReadExponent("e7", 0) == 7.0 ? 0 : 1;
}
