// EXPECT_EXIT: 0

#include <utility>

template<typename T> struct Identity;
template<> struct Identity<float>
{
  static inline float Value() { return 1.0f; }
};

template<typename T> float One()
{
  return Identity<std::remove_reference<T>::type>::Value();
}

int main()
{
  return One<float>() == 1.0f ? 0 : 1;
}
