namespace Detail {
template<class T> struct Holder {
  T value;
  struct Inner { struct Leaf { int value; }; };
};
}
template<int N> struct Outer { struct Inner { struct Leaf { int value; }; }; };
template<int N> int read() {
  class Outer<N>::Inner::Leaf leaf = { N };
  return leaf.value;
}
int main() {
  struct Detail::Holder<int> item = { 19 };
  struct Detail::Holder<int>::Inner::Leaf leaf = { 23 };
  return item.value + leaf.value + read<7>() == 49 ? 0 : 1;
}
