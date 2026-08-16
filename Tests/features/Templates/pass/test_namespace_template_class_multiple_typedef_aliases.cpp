// EXPECT_EXIT: 0

namespace ns_alias
{
  template<class T>
  class AliasBox
  {
  public:
    typedef T value_type;
    typedef const T &reference;
    typedef unsigned long long size_type;
    typedef const T *iterator;

  private:
    AliasBox(iterator p, size_type n) : data(p), count(n) {}

  public:
    iterator data;
    size_type count;

    AliasBox() : data(0), count(0) {}

    size_type size() const { return count; }
  };
}

int main(void)
{
  ns_alias::AliasBox<int> box;
  return box.size() == 0 ? 0 : 1;
}
