namespace sample {
template<class T> struct Value { T number; };
template<class T> int read(const Value<T> &v) { return v.number; }
}
int main() {
  sample::Value<int> v = {9};
  int Value = 3;
  return sample::read<int>(v) + Value != 12;
}
