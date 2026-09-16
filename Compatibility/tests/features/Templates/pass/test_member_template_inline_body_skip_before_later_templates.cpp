template <typename T>
struct Vec
{
  T x;
};

template <typename T>
struct Mat
{
  template <typename U> static Mat<T> Create(const T &m0, const T &m1)
  {
    return Mat<T>(m0, m1);
  }

  template <typename U> auto operator *(const Vec<U> &vec) const
  {
    if (vec.x < T())
      return Vec<T>();
    return Vec<T>();
  }

  template <typename U> auto operator +(const Mat<U> &rhs) const;
};

int main()
{
  return 0;
}
