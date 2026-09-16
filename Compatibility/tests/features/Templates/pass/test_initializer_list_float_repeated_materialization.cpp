#include <initializer_list>

template <typename T>
struct FloatList
{
  FloatList(const std::initializer_list<T> &values)
  {
    (void)values.begin();
  }
};

static void materialize_float_lists()
{
  FloatList<float> one(std::initializer_list<float>{ 1.0f, 2.0f });
  FloatList<float> two(std::initializer_list<float>{ 3.0f, 4.0f });
  (void)one;
  (void)two;
}

int main()
{
  return 0;
}
