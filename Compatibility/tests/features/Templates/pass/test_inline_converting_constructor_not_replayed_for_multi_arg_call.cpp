template <typename T>
struct ReplayVector2
{
  T x, y;

  ReplayVector2(const T &xValue, const T &yValue)
    : x(xValue), y(yValue)
  {
  }

  template <typename U, typename V>
  explicit ReplayVector2(const U &xValue, const V &yValue)
    : x(T(xValue)), y(T(yValue))
  {
  }

  template <typename U>
  explicit ReplayVector2(const ReplayVector2<U> other)
    : x(T(other.x)), y(T(other.y))
  {
  }
};

ReplayVector2<double> makeVector(double x, double y)
{
  return { x, y };
}

int main()
{
  ReplayVector2<double> value = makeVector(12.5, 27.25);
  return value.x == 12.5 && value.y == 27.25 ? 0 : 1;
}
