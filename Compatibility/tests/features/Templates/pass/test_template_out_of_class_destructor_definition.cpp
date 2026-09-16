// EXPECT_EXIT: 0

template<typename T>
struct Box
{
  T value;
  Box() : value(0) {}
  Box(T v) : value(v) {}
  ~Box();
};

template<typename T>
Box<T>::~Box()
{
  value = 0;
}

int main()
{
  Box<int> a(7);
  Box<char> b('x');
  return a.value == 7 && b.value == 'x' ? 0 : 1;
}
