// EXPECT_COMPILE_ARGS: -Werror
template<class T> struct Sequence {
  int sum;
  Sequence(): sum(0) {}
  void append(T&& value);
  void append(const T& value);
  void append(const Sequence<T>& values);
  void append(Sequence<T>&& values);
  template<int N> void append(const T (&values)[N]);
};
template<class T> void Sequence<T>::append(T&& value) {
  sum += value.value * 2;
}
template<class T> void Sequence<T>::append(const T& value) {
  sum += value.value;
}
template<class T> void Sequence<T>::append(const Sequence<T>& values) {
  sum += values.sum;
}
template<class T> void Sequence<T>::append(Sequence<T>&& values) {
  sum += values.sum * 2;
}
template<class T> template<int N>
void Sequence<T>::append(const T (&values)[N]) {
  for (int i = 0; i < N; ++i) append(values[i]);
}

struct Text {
  int value;
  explicit Text(int v = 3): value(v) {}
  // This publishes Sequence<Text> before Text is complete.
  typedef Sequence<Text> Parts;
  static void inspect(const Parts&);
  Parts split();
};

int main() {
  Text::Parts a, b;
  const Text text(7);
  a.append(text);
  a.append(Text(4));
  b.append(a);
  const Text values[2] = {Text(2), Text(5)};
  b.append(values);
  return a.sum != 15 || b.sum != 22;
}
