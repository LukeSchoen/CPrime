template<typename T>
class Vector2
{
public:
  T x;
  T y;

  Vector2(T xValue, T yValue) : x(xValue), y(yValue) {}

  template<typename U> auto operator*(const U &value) const;
  template<typename U> auto operator*(const Vector2<U> &value) const;
};

template<typename T>
template<typename U>
auto Vector2<T>::operator*(const U &value) const
{
  return Vector2<T>(T(x * value), T(y * value));
}

template<typename T>
template<typename U>
auto Vector2<T>::operator*(const Vector2<U> &value) const
{
  return Vector2<T>(T(x * value.x), T(y * value.y));
}

int main()
{
  Vector2<float> left(2.0f, 3.0f);
  Vector2<int> right(4, 5);
  auto result = left * right;
  return result.x == 8.0f && result.y == 15.0f ? 0 : 1;
}
