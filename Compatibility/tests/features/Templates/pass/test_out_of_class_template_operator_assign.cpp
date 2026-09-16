// EXPECT_EXIT: 0

template<typename T>
struct Vec
{
  T value;
  Vec() : value(0) {}
  Vec(T v) : value(v) {}
  Vec &operator=(const Vec &rhs);
};

template<typename T>
Vec<T> &Vec<T>::operator=(const Vec<T> &rhs)
{
  value = rhs.value;
  return *this;
}

int main()
{
  Vec<int> a(3);
  Vec<int> b(5);
  a = b;
  return a.value == 5 ? 0 : 1;
}
