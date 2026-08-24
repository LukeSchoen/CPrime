class Value
{
public:
  Value(int value) : m_value(value) { }

  static int Read(const char *left, const char *right)
  {
    return left[0] - right[0];
  }

  int Read(const Value &other) const
  {
    return m_value - other.m_value;
  }

  static Value Resolve(const Value &value)
  {
    return value;
  }

  Value Resolve() const
  {
    return *this;
  }

  int m_value;
};

int main()
{
  Value left(9);
  Value right(4);
  Value copy = left.Resolve();
  if (left.Read(right) != 5)
    return 1;
  if (copy.m_value != 9)
    return 2;
  return Value::Read("b", "a") != 1;
}
