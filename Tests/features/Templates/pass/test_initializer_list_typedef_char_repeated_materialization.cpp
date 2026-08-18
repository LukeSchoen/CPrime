#include <initializer_list>

typedef signed char small_i8;

template <typename T>
struct TypedefList
{
  TypedefList(const std::initializer_list<T> &values)
  {
    (void)values.begin();
  }
};

static void materialize_typedef_lists()
{
  TypedefList<small_i8> one(std::initializer_list<small_i8>{ 1, 2 });
  TypedefList<small_i8> two(std::initializer_list<small_i8>{ 3, 4 });
  (void)one;
  (void)two;
}

int main()
{
  return 0;
}
