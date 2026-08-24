template <typename T>
struct MixedVector3
{
  T x, y, z;

  MixedVector3(const T &xValue, const T &yValue, const T &zValue)
    : x(xValue), y(yValue), z(zValue)
  {
  }

  template <typename U, typename V, typename W>
  explicit MixedVector3(const U &xValue, const V &yValue, const W &zValue)
    : x(T(xValue)), y(T(yValue)), z(T(zValue))
  {
  }
};

int main()
{
  float height = 48.5f;
  MixedVector3<float> values[] = {
    MixedVector3<float>(0, 0, 0),
    MixedVector3<float>(0, height, 0)
  };
  return values[0].y == 0.0f && values[1].y == 48.5f ? 0 : 1;
}
