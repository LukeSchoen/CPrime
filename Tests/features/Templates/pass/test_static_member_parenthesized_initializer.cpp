// EXPECT_EXIT: 0
int initialized;
int next_value() { return ++initialized; }
template<class T> struct Values {
  static const int first;
  static int second;
  static int method(int value);
};
template<class T> const int Values<T>::first(next_value());
template<class T> int Values<T>::second(next_value());
template<class T> int Values<T>::method(int value) { return value + 1; }
template const int Values<int>::first;
template int Values<int>::second;
int forced;
template<class T> struct Forced { static int unused; };
template<class T> int Forced<T>::unused(++forced);
template int Forced<int>::unused;
int main() {
  Values<int> object;
  int total = Values<int>::first + Values<int>::second
      + Values<char>::first + Values<char>::second;
  if (object.first != Values<int>::first || object.second != Values<int>::second) return 2;
  return initialized != 4 || forced != 1 || total != 10 || Values<int>::method(3) != 4;
}
