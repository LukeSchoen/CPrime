// EXPECT_EXIT: 0
// A static factory returning the class template repeats the class name after
// `static`, which used to make the constructor definitions look like duplicate
// definitions of that static member.
template <typename T> struct Box
{
  T value;
  Box();
  Box(const T &v);
  static Box<T> Zero();
};

template <typename T> Box<T>::Box() : value(0) {}
template <typename T> Box<T>::Box(const T &v) : value(v) {}
template <typename T> Box<T> Box<T>::Zero() { return Box<T>(); }

int main()
{
  Box<int> first;
  Box<int> second(7);
  if (first.value != 0) return 1;
  if (second.value != 7) return 2;
  if (Box<int>::Zero().value != 0) return 3;
  return 0;
}
