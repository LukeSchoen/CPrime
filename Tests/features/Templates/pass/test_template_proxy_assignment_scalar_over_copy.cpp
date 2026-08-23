template<typename T>
class clBitRef
{
public:
  clBitRef &operator=(const clBitRef &rhs);
  clBitRef &operator=(bool const &rhs);
  operator bool() const;

  T value;
};

template<typename T>
clBitRef<T> &clBitRef<T>::operator=(bool const &rhs)
{
  value = rhs ? 1 : 0;
  return *this;
}

template<typename T>
clBitRef<T> &clBitRef<T>::operator=(const clBitRef &rhs)
{
  return *this = bool(rhs);
}

template<typename T>
clBitRef<T>::operator bool() const
{
  return value != 0;
}

int main()
{
  clBitRef<unsigned int> low;
  clBitRef<unsigned int> high;

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
