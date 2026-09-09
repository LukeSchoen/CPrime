struct Unrelated {
  template<class T> operator T() { return T::missing; }
};
template<class T> struct Value {
  template<class U> operator U() { return U(T(7)); }
};
struct Direct {
  template<class T> operator T() { return T(11); }
};
int early() { Value<int> v; return v; }
struct Later {
  template<class T> operator T() { return T(13); }
};
int main() {
  Value<short> v;
  Direct d;
  Later l;
  long a = v;
  int b = d, c = l;
  return early() != 7 || a != 7 || b != 11 || c != 13;
}
