template<typename T>
struct Pair
{
  T first;
  T second;

  Pair(const T &a, const T &b) : first(a), second(b) {}
  static Pair Make(const T &value);
};

template<typename T>
Pair<T> Pair<T>::Make(const T &value)
{
  return Pair { value, value + 1 };
}

int main()
{
  Pair<int> pair = Pair<int>::Make(4);
  (void)pair;
  return 0;
}
