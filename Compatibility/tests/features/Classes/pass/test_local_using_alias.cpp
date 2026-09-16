template <typename T>
T zero_value() { return T(); }

template <typename T>
T twice(T value)
{
  using Element = T;
  Element copy = value;
  Element zero = zero_value<Element>();
  return copy + value + zero;
}

int main()
{
  return twice<int>(3) == 6 ? 0 : 1;
}
