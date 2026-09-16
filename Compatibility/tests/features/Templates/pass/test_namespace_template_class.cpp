// Consolidated from similar standalone regressions; each cpc_case_N preserves one.
#include <utility>

namespace cpc_case_0
{
// EXPECT_EXIT: 0

namespace ns_param
{
  template<class T>
  class Box
  {
  public:
    T value;
  };
}

int read_box(const ns_param::Box<int> &box)
{
  return box.value;
}

int run(void)
{
  ns_param::Box<int> box;
  box.value = 9;
  return read_box(box) - 9;
}
}

namespace cpc_case_1
{
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

int run(void)
{
  ns_alias::AliasBox<int> box;
  return box.size() == 0 ? 0 : 1;
}
}

namespace cpc_case_2
{

template<typename T>
int use_trait()
{
    if (std::is_trivially_copyable<T>::value)
        return 1;
    return 0;
}

int run()
{
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
