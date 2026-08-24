#include <type_traits>

template<typename T>
struct Vector3
{
  T values[3];

  unsigned long long size() const { return 3; }
  const T &operator[](const long long &index) const { return values[index]; }
};

template<typename T>
T zeroValue()
{
  return T();
}

template<typename T>
auto vectorSum(const T &vector);

template<typename T>
auto vectorSum(const T &vector)
{
  auto sum = zeroValue<std::remove_const_t<
    std::remove_reference_t<decltype(vector[0])>>>()
    + zeroValue<std::remove_const_t<
        std::remove_reference_t<decltype(vector[0])>>>();
  for (unsigned long long i = 0; i < vector.size(); ++i)
    sum += vector[i];
  return sum;
}

int main()
{
  Vector3<float> values;
  values.values[0] = 1.0f;
  values.values[1] = 0.09f;
  values.values[2] = 0.0049f;
  float result = vectorSum(values);
  return result == 1.0949f ? 0 : 1;
}

// EXPECT_EXIT: 0
