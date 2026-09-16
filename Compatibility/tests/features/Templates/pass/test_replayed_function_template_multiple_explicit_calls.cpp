template<typename T>
T unit_value()
{
  return (T)1;
}

template<typename T>
T twice_unit_value(const T &value)
{
  return value * unit_value<T>() + unit_value<T>();
}

int main()
{
  return twice_unit_value(20) == 21 ? 0 : 1;
}
