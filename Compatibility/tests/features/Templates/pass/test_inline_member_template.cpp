// Consolidated from similar standalone regressions; each cpc_case_N preserves one.

namespace cpc_case_0
{
template<typename T>
struct Vec4
{
  T x;
  T y;
  T z;
  T w;
};

static int scalar_called;

template<typename T>
Vec4<T> makeVector(const T &x, const T &y, const T &z, const T &w)
{
  Vec4<T> result;
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
  return result;
}

template<typename T>
struct Matrix4x4
{
  T x;

  Matrix4x4() : x(0) {}
  Matrix4x4(const T &value) : x(value) {}

  template<typename U>
  auto operator*(const Vec4<U> &value) const
  {
    return makeVector(x * value.x, x * value.y,
                          x * value.z, x * value.w);
  }

  template<typename U>
  auto operator*(const U &scale) const
  {
    scalar_called = 1;
    return Matrix4x4<U>(x * scale);
  }

  Matrix4x4<T> Scaled() const
  {
    return (*this) * T(2);
  }
};

int run()
{
  Matrix4x4<double> matrix(3.0);
  Vec4<double> vector;
  vector.x = 2.0;
  vector.y = 3.0;
  vector.z = 4.0;
  vector.w = 5.0;
  Vec4<double> result = matrix * vector;
  if (scalar_called)
    return 1;
  Matrix4x4<double> scaled = matrix.Scaled();
  return scalar_called && result.x == 6.0 && result.w == 15.0
         && scaled.x == 6 ? 0 : 1;
}
}

namespace cpc_case_1
{
template <typename T> struct Vec3
{
  T x;
};

template <typename T> struct Vec4
{
  T x;

  Vec3<T> XYZ() const
  {
    Vec3<T> v;
    v.x = x;
    return v;
  }
};

template <typename T> Vec4<T> makeVector(const T &x, const T &, const T &, const T &)
{
  Vec4<T> v;
  v.x = x;
  return v;
}

template <typename T> struct Matrix
{
  template <typename U> auto operator*(const Vec4<U> &v) const
  {
    return makeVector(v.x + T(2), v.x, v.x, v.x);
  }
};

int run()
{
  Matrix<double> m;
  Vec4<double> v;
  v.x = 5;
  Vec3<double> out = (m * v).XYZ();
  return out.x == 7 ? 0 : 1;
}
}

namespace cpc_case_2
{
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

int run()
{
  float height = 48.5f;
  MixedVector3<float> values[] = {
    MixedVector3<float>(0, 0, 0),
    MixedVector3<float>(0, height, 0)
  };
  return values[0].y == 0.0f && values[1].y == 48.5f ? 0 : 1;
}
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  return 0;
}
