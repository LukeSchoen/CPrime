// EXPECT_EXIT: 0

namespace sample
{
  enum class Flags
  {
    None = 0,
    SkipInitialBuild = 1
  };

  inline int operator&(Flags lhs, Flags rhs)
  {
    return static_cast<int>(lhs) & static_cast<int>(rhs);
  }

  template<class T>
  struct Index
  {
    bool should_build(Flags flags)
    {
      return !(flags & Flags::SkipInitialBuild);
    }
  };
}

int main()
{
  sample::Index<int> index;
  return index.should_build(sample::Flags::None) ? 0 : 1;
}
