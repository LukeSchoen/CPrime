// EXPECT_EXIT: 0
template<class T> struct Base { int read() { return 1; } };
template<class T> struct Value : Base<T> {
  int number;
  int read() { return 2; }
  using Base<T>::read;
  int check() { return Base<T>::read() + Value<T>::read() + Value<T>::number; }
};
int main() { Value<int> value; value.number = 4; return value.check() != 7; }
