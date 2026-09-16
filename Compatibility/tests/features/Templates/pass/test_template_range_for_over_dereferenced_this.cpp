template<typename T>
struct Range
{
  T values[3];

  int Size() const
  {
    return 3;
  }

  const T &operator[](int index) const
  {
    return values[index];
  }

  T Sum() const;
};

template<typename T>
T Range<T>::Sum() const
{
  T result = 0;
  for (const auto &value : *this)
    result += value;
  return result;
}

int main()
{
  Range<int> range;
  range.values[0] = 3;
  range.values[1] = 5;
  range.values[2] = 7;
  return range.Sum() == 15 ? 0 : 1;
}
