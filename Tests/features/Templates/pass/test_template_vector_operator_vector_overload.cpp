// EXPECT_EXIT: 0

template<typename T> struct Vec2
{
  T x, y;
  Vec2() : x(0), y(0) {}
  Vec2(T a, T b) : x(a), y(b) {}
  template<typename U> auto operator-(const U &val) const;
  template<typename U> auto operator-(const Vec2<U> &o) const;
};
template<typename T> template<typename U>
auto Vec2<T>::operator-(const U &val) const { return clCreateVector(x - val, y - val); }
template<typename T> template<typename U>
auto Vec2<T>::operator-(const Vec2<U> &o) const { return clCreateVector(x - o.x, y - o.y); }
template<typename T> auto clCreateVector(const T &x, const T &y) { return Vec2<T>(x, y); }
int main()
{
  Vec2<int> a(1,2), b(0,0);
  Vec2<int> c = a - b;
  return c.x == 1 && c.y == 2 ? 0 : 1;
}
