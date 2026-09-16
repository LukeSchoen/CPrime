template <typename T>
struct ExplicitRuntimeVec
{
  T x;
  T y;
  ExplicitRuntimeVec() = default;
  ExplicitRuntimeVec(const T &_x, const T &_y) : x(_x), y(_y) {}
  template <typename U>
  ExplicitRuntimeVec<T> operator-(const ExplicitRuntimeVec<U> &other) const;
};

template <typename T>
template <typename U>
ExplicitRuntimeVec<T>
ExplicitRuntimeVec<T>::operator-(const ExplicitRuntimeVec<U> &other) const
{
  return ExplicitRuntimeVec<T>(x - other.x, y - other.y);
}

int main()
{
  auto result = ExplicitRuntimeVec<int>(7, 8) - ExplicitRuntimeVec<int>(1, 2);
  return result.x != 6 || result.y != 6;
}
