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

int main()
{
  StringLike a;
  StringLike b;
  a.value = 3;
  b.value = 2;
  return a.DeleteUptoAndIncluding(b) && a.value == 2 ? 0 : 1;
}
