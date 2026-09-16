// EXPECT_EXIT: 0
#include <type_traits>
#include <utility>

namespace detection
{
  template<class T, class = int>
  struct has_fill : std::false_type {};

  template<class T>
  struct has_fill<T, decltype((void)std::declval<T>().fill(1, 0), 0)>
      : std::true_type {};

  template<class Container, class Value>
  typename std::enable_if<has_fill<Container>::value, void>::type
  fill(Container &container, unsigned count, const Value &value)
  {
    container.fill(count, value);
  }

  template<class Container, class Value>
  typename std::enable_if<!has_fill<Container>::value, void>::type
  fill(Container &container, unsigned count, const Value &value)
  {
    for (unsigned i = 0; i < count; ++i) container[i] = value;
  }

  struct Dynamic
  {
    int values[3];

    void fill(unsigned count, int value)
    {
      for (unsigned i = 0; i < count; ++i) values[i] = value;
    }
  };
}

int main()
{
  detection::Dynamic values = {};
  detection::fill(values, 3, 7);
  return values.values[0] == 7 && values.values[2] == 7 ? 0 : 1;
}
