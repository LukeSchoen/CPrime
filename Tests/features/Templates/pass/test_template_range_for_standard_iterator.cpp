struct IntIterator
{
  int *position;

  IntIterator(int *value) : position(value) {}

  int &operator*() const
  {
    return *position;
  }

  IntIterator &operator++()
  {
    ++position;
    return *this;
  }

  bool operator!=(const IntIterator &other) const
  {
    return position != other.position;
  }
};

template<typename T>
struct Iterable
{
  T values[3];

  IntIterator begin()
  {
    return IntIterator(values);
  }

  IntIterator end()
  {
    return IntIterator(values + 3);
  }
};

int main()
{
  Iterable<int> values;
  values.values[0] = 2;
  values.values[1] = 4;
  values.values[2] = 8;
  int sum = 0;
  for (auto &value : values)
    sum += value;
  return sum == 14 ? 0 : 1;
}
