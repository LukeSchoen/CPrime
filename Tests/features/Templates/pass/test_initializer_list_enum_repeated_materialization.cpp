#include <initializer_list>

enum Key
{
  KeyA,
  KeyB
};

template <typename T>
struct List
{
  List() {}
  List(const std::initializer_list<T> &values)
  {
    (void)values.begin();
  }
};

static void materialize_lists()
{
  List<Key> one(std::initializer_list<Key>{ KeyA, KeyB });
  List<Key> two(std::initializer_list<Key>{ KeyB, KeyA });
  (void)one;
  (void)two;
}

int main()
{
  return 0;
}
