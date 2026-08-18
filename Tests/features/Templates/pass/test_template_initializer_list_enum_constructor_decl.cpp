#include <initializer_list>

enum CtorKey
{
  CtorKeyA,
  CtorKeyB
};

template <typename T>
struct CtorList
{
  CtorList() {}
  CtorList(const std::initializer_list<T> &values)
  {
    (void)values.begin();
  }
};

int main()
{
  CtorList<CtorKey> list;
  (void)list;
  return 0;
}
