template<typename T>
struct clVector4
{
  T x;
  T y;
  T z;
  T w;
};

static int scalar_called;

template<typename T>
clVector4<T> clCreateVector(const T &x, const T &y, const T &z, const T &w)
{
  clVector4<T> result;
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
  return result;
}

template<typename T>
struct clMatrix4x4
{
  T x;

  clMatrix4x4() : x(0) {}
  clMatrix4x4(const T &value) : x(value) {}

  template<typename U>
  auto operator*(const clVector4<U> &value) const
  {
    return clCreateVector(x * value.x, x * value.y,
                          x * value.z, x * value.w);
  }

  template<typename U>
  auto operator*(const U &scale) const
  {
    scalar_called = 1;
    return clMatrix4x4<U>(x * scale);
  }

  clMatrix4x4<T> Scaled() const
  {
    return (*this) * T(2);
  }
};

int main()
{
  clMatrix4x4<double> matrix(3.0);
  clVector4<double> vector;
  vector.x = 2.0;
  vector.y = 3.0;
  vector.z = 4.0;
  vector.w = 5.0;
  clVector4<double> result = matrix * vector;
  if (scalar_called)
    return 1;
  clMatrix4x4<double> scaled = matrix.Scaled();
  return scalar_called && result.x == 6.0 && result.w == 15.0
         && scaled.x == 6 ? 0 : 1;
}
