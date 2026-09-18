template<typename T> struct Vec2;
template<typename T> Vec2<T> makeVector(const T &x, const T &y);

template<typename T> struct Vec2
{
  T x, y;
  Vec2() : x(0), y(0) {}
  Vec2(T a, T b) : x(a), y(b) {}
  template<typename U> auto operator-(const U &val) const;
  template<typename U> auto operator-(const Vec2<U> &o) const;
};
template<typename T> template<typename U>
auto Vec2<T>::operator-(const U &val) const { return makeVector(x - val, y - val); }
template<typename T> template<typename U>
auto Vec2<T>::operator-(const Vec2<U> &o) const { return makeVector(x - o.x, y - o.y); }
template<typename T> Vec2<T> makeVector(const T &x, const T &y) { return Vec2<T>(x, y); }
int main()
{
  Vec2<int> a(1,2), b(0,0);
  Vec2<int> c = a - 1;
  return c.x != 0 || c.y != 1;
}
