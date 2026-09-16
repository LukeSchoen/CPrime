// Consolidated from similar standalone regressions; each cpc_case_N preserves one.

namespace cpc_case_0
{
struct CameraField
{
  int value;

  void Set(int v)
  {
    value = v;
  }
};

int run()
{
  return 0;
}
}

namespace cpc_case_1
{
// EXPECT_EXIT: 0

class StringLike
{
public:
  int value;

  int Length() const { return value; }
  int IndexOf(const StringLike &target) const { return target.value; }

  bool DeleteUptoAndIncluding(const StringLike &target)
  {
    int index = IndexOf(target);
    if (index == Length())
      return false;
    value = index;
    return true;
  }
};

int run()
{
  StringLike a;
  StringLike b;
  a.value = 3;
  b.value = 2;
  return a.DeleteUptoAndIncluding(b) && a.value == 2 ? 0 : 1;
}
}

namespace cpc_case_2
{
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

int run()
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
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  return 0;
}
