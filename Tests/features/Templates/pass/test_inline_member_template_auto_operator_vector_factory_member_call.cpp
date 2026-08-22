template <typename T> struct clVector3
{
  T x;
};

template <typename T> struct clVector4
{
  T x;

  clVector3<T> XYZ() const
  {
    clVector3<T> v;
    v.x = x;
    return v;
  }
};

template <typename T> clVector4<T> clCreateVector(const T &x, const T &, const T &, const T &)
{
  clVector4<T> v;
  v.x = x;
  return v;
}

template <typename T> struct Matrix
{
  template <typename U> auto operator*(const clVector4<U> &v) const
  {
    return clCreateVector(v.x + T(2), v.x, v.x, v.x);
  }
};

int main()
{
  Matrix<double> m;
  clVector4<double> v;
  v.x = 5;
  clVector3<double> out = (m * v).XYZ();
  return out.x == 7 ? 0 : 1;
}
