#include <initializer_list>

enum SigKey
{
  SigKeyA,
  SigKeyB
};

template <typename T>
struct SigList
{
  void assign(const std::initializer_list<T> &values)
  {
    (void)values.begin();
  }
};

int main()
{
  SigList<SigKey> list;
  (void)list;
  return 0;
}
