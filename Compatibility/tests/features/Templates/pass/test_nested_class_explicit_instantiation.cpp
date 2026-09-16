// EXPECT_EXIT: 0
template<class T> struct Outer {
  enum Choice { first, second };
  template<Choice N> struct Inner {
    int read() { return N + sizeof(T); }
    template<class U> struct Deep { int size() { return sizeof(U); } };
  };
};
template struct Outer<int>;
template struct Outer<char>::Inner<Outer<char>::second>;
template struct Outer<int>::Inner<Outer<int>::first>::Deep<char>;
int main() {
  Outer<char>::Inner<Outer<char>::second> value;
  Outer<int>::Inner<Outer<int>::first>::Deep<char> deep;
  return value.read() != 2 || deep.size() != 1;
}
