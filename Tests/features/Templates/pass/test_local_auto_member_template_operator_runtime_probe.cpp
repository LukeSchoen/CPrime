template <typename T>
struct LocalAutoRuntimeVec
{
  T x;
  T y;
  LocalAutoRuntimeVec(const T &_x, const T &_y) : x(_x), y(_y) {}
  template <typename U>
  auto operator-(const LocalAutoRuntimeVec<U> &other) const;
};

template <typename T>
template <typename U>
auto LocalAutoRuntimeVec<T>::operator-(const LocalAutoRuntimeVec<U> &other) const
{
  LocalAutoRuntimeVec<T> result = { x - other.x, y - other.y };
  return result;
}

int main()
{
  LocalAutoRuntimeVec<int> lhs = { 7, 8 };
  LocalAutoRuntimeVec<int> rhs = { 1, 2 };
  auto result = lhs - rhs;
  return result.x;
}
