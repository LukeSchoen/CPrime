#include <type_traits>

struct SmallVector
{
  double values[3];

  int size() const { return 3; }
  double operator[](int index) const { return values[index]; }
};

template<typename T> T zero_value()
{
  return T();
}

template<typename T> auto vector_sum(const T &value);

template<typename T> auto vector_sum(const T &vector)
{
  auto sum = zero_value<std::remove_const_t<
    std::remove_reference_t<decltype(vector[0])>>>()
    + zero_value<std::remove_const_t<
      std::remove_reference_t<decltype(vector[0])>>>();
  return sum + vector[0] + vector[1] + vector[2];
}

int main()
{
  SmallVector a;
  a.values[0] = 1.0;
  a.values[1] = 2.0;
  a.values[2] = 3.0;
  return vector_sum(a) == 6.0 ? 0 : 1;
}
