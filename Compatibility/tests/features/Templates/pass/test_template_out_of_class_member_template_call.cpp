// EXPECT_EXIT: 0

template<typename T>
struct V
{
  T x;
  V(const T &v) : x(v) {}
  template<typename U> V Add(const V<U> &rhs) const;
};

template<typename T>
template<typename U>
V<T> V<T>::Add(const V<U> &rhs) const
{
  return V<T>(x + (T)rhs.x);
}

int main()
{
  V<float> a(1);
  V<double> b(2);
  V<float> c = a.Add(b);
  return c.x == 3.0f ? 0 : 1;
}
