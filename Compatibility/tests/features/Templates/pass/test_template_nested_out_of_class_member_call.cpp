// EXPECT_EXIT: 0

template<typename T>
struct Box
{
  T value;
  T get() const;
  T add(T other) const;
};

template<typename T>
T Box<T>::get() const { return value; }

template<typename T>
T Box<T>::add(T other) const { return get() + other; }

int main()
{
  Box<int> b;
  b.value = 10;
  return b.add(32) == 42 ? 0 : 1;
}
