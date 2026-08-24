template<typename T>
class Bag
{
public:
  class Iterator
  {
  public:
    Iterator(const T *p) : m_p(p) {}
    Iterator &operator++() { ++m_p; return *this; }
    bool operator!=(const Iterator &other) const { return m_p != other.m_p; }
    const T &operator*() const { return *m_p; }

  private:
    const T *m_p;
  };

  Bag(T a, T b) { m_values[0] = a; m_values[1] = b; }
  Iterator begin() const { return Iterator(m_values); }
  Iterator end() const { return Iterator(m_values + 2); }

private:
  T m_values[2];
};

int Sum(const Bag<int> &values)
{
  int result = 0;
  for (const auto &value : values)
    result += value;
  return result;
}

int main()
{
  Bag<int> values(4, 7);
  return Sum(values) == 11 ? 0 : 1;
}
