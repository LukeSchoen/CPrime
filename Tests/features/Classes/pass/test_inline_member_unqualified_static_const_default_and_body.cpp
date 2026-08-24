typedef unsigned long size_t;

class TextLike
{
public:
  static const size_t npos = (size_t)-1;

  size_t find(size_t pos = npos) const
  {
    if (pos == npos)
      return 0;
    return npos;
  }
};

int main()
{
  TextLike text;
  if (TextLike::npos != (size_t)-1)
    return 3;
  if (text.find((size_t)-1) != 0)
    return 4;
  if (text.find() != 0)
    return 1;
  if (text.find(2) != TextLike::npos)
    return 2;
  return 0;
}
