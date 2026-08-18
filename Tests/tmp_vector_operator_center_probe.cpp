template <typename T>
struct CenterVector2
{
  T x;
  T y;

  CenterVector2() = default;
  CenterVector2(const T &_x, const T &_y) : x(_x), y(_y) {}

  template <typename U> auto operator +(const CenterVector2<U> &o) const { return CenterVector2<T>(x + o.x, y + o.y); }
  template <typename U> auto operator /(const U &val) const { return CenterVector2<T>(x / val, y / val); }
};

template <typename T>
struct CenterBox
{
  typedef CenterVector2<T> VertexType;

  VertexType Center() const;

  VertexType min;
  VertexType max;
};

template <typename T>
CenterVector2<T> CenterBox<T>::Center() const
{
  return (min + max) / 2;
}

int main()
{
  CenterBox<float> box;
  (void)box;
  return 0;
}
