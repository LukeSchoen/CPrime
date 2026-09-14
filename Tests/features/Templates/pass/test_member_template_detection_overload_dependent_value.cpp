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
    float values[3];

    void fill(unsigned count, float value)
    {
      for (unsigned i = 0; i < count; ++i) values[i] = value;
    }
  };

  template<class Container>
  struct Algorithm
  {
    template<class Result>
    bool run(Result &result)
    {
      Container values;
      auto zero = static_cast<decltype(result.limit())>(0);
      fill(values, 3, zero);
      return values.values[0] == 0 && values.values[2] == 0;
    }
  };

  struct Result
  {
    float limit() const { return 1.0f; }
  };
}

int main()
{
  detection::Result result;
  detection::Algorithm<detection::Dynamic> algorithm;
  return algorithm.run(result) ? 0 : 1;
}
