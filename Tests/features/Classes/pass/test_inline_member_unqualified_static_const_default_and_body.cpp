typedef unsigned long size_t;

class TextLike
{
public:
  static const size_t npos = 3;

  size_t find(size_t pos = npos) const
  {
    if (pos == npos)
      return 0;
    return npos;
  }
};

int main()
{
  return 0;
}
