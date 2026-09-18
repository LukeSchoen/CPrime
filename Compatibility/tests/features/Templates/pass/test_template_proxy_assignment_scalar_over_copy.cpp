template<typename T>
class BitRef
{
public:
  BitRef &operator=(const BitRef &rhs);
  BitRef &operator=(bool const &rhs);
  operator bool() const;

  T value;
};

template<typename T>
BitRef<T> &BitRef<T>::operator=(bool const &rhs)
{
  value = rhs ? 1 : 0;
  return *this;
}

template<typename T>
BitRef<T> &BitRef<T>::operator=(const BitRef &rhs)
{
  return *this = bool(rhs);
}

template<typename T>
BitRef<T>::operator bool() const
{
  return value != 0;
}

int main()
{
  BitRef<unsigned int> low;
  BitRef<unsigned int> high;

  high = true;
  if (high.value != 1)
    return 1;
  low = high;
  if (low.value != 1)
    return 2;
  high = false;
  if (high.value != 0)
    return 3;
  return 0;
}
