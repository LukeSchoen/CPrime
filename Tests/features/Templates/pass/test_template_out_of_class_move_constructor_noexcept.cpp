template<typename T>
struct Box
{
  T value;

  Box();
  Box(Box &&other) noexcept;
};

template<typename T>
Box<T>::Box() : value(0) {}

template<typename T>
Box<T>::Box(Box &&other) noexcept : value(other.value)
{
  other.value = 0;
}

int main()
{
  Box<int> first;
  return first.value;
}
