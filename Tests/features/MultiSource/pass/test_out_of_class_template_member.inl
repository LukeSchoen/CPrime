template<typename T>
void OutOfClassBox<T>::Set(const T &next)
{
  value = next;
}

template<typename T>
T OutOfClassBox<T>::Get() const
{
  return value;
}

template<typename T>
void OutOfClassBox<T>::Clear()
{
  value = 0;
}
