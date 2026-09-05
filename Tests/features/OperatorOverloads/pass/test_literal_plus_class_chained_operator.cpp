class SmallString
{
public:
  SmallString();
  SmallString(const char *text);
  template<typename T> explicit SmallString(const T &value) : len(1000) {}
  SmallString operator+(const SmallString &rhs) const;
  int Length() const;

  int len;
};

SmallString::SmallString() : len(0) {}
SmallString::SmallString(const char *text)
{
  len = 0;
  while (text[len])
    ++len;
}

SmallString SmallString::operator+(const SmallString &rhs) const
{
  SmallString out;
  out.len = len + rhs.len;
  return out;
}

int SmallString::Length() const { return len; }

SmallString operator+(const char *left, const SmallString &right)
{
  return SmallString(left) + right;
}

int main()
{
  SmallString middle("data");
  SmallString result = "No " + middle + ": " + SmallString("name");
  return result.Length() == 13 ? 0 : 1;
}
