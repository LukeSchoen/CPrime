template <typename T>
struct LocalAutoCtorVec
{
  T x;
  T y;

  LocalAutoCtorVec() = default;
  LocalAutoCtorVec(const T &_x, const T &_y) : x(_x), y(_y) {}

  template <typename U>
  auto operator-(const LocalAutoCtorVec<U> &other) const;
};

template <typename T>
template <typename U>
auto LocalAutoCtorVec<T>::operator-(const LocalAutoCtorVec<U> &other) const
{
  return LocalAutoCtorVec<T>(x - other.x, y - other.y);
}

typedef LocalAutoCtorVec<int> LocalAutoCtorVecI;

struct LocalAutoWindow
{
  LocalAutoWindow(const LocalAutoCtorVecI &size) : value(size.x) {}
  int value;
};

int main()
{
  auto res = LocalAutoCtorVecI(7, 8) - LocalAutoCtorVecI(1, 2);
  LocalAutoWindow window(res);
  return window.value != 6;
}
