template<typename T>
struct BitProxy
{
  T *value;

  BitProxy(T &target) : value(&target) {}
  operator bool() const { return true; }
};

template<typename T>
struct BitSource
{
  T value;

  BitProxy<T> operator[](int)
  {
    BitProxy<T> result(value);
    return result;
  }
  bool operator[](int) const { return value != 0; }
};

int main()
{
  BitSource<int> left;
  BitSource<int> right;
  const BitSource<int> &other = right;
  left.value = 1;
  right.value = 0;

  return (left[0] ^ other[0]) ? 0 : 1;
}
