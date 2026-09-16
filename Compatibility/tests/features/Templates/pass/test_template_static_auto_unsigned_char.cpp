#include <type_traits>

template<typename T, bool Signed = false> struct _clAbsHelper
{
  static auto Abs(const T &value) { return value; }
};

template<typename T> struct _clAbsHelper<T, true>
{
  static auto Abs(const T &value) { return value < 0 ? -value : value; }
};

template<typename T> auto absolute_value(const T &value);
template<typename T> struct Vector2 { T x, y; };
template<typename T> auto absolute_value(const Vector2<T> &value);

template<typename T> auto absolute_value(const T &value)
{
  return _clAbsHelper<T, std::is_signed<T>::value>::Abs(value);
}

template<typename T> auto absolute_value(const Vector2<T> &value)
{
  return Vector2<T>{absolute_value(value.x), absolute_value(value.y)};
}

int main()
{
  unsigned char value = 42;
  return absolute_value(value) == 42 ? 0 : 1;
}
