// Static auto return deduction uses complete bodies and normal overload lookup.
float transform(float n) { return n + 0.25f; }
double transform(double n) { return n + 0.5; }
int transform(int n) { return n + 10; }

template<class T> struct Operations {
  static auto unused(T value) { return value.nonexistent_member(); }
  static auto apply(const T& value) { return transform(value); }
  static auto forward(const T& value) { return later(value); }
  static auto later(const T& value) { T answer = value; return transform(answer); }
  static auto sum(const T& value) {
    double result = 0.5;
    for (int i = 0; i < 3; ++i) result += value;
    return result;
  }
};
template<class T> auto forward(const T& value) { return Operations<T>::apply(value); }
struct Product { double value; };
struct Factory {
  static auto product(double value) { Product p = {value}; return p; }
  static auto nothing() { }
  static auto store(int* output) { *output = 7; }
  static auto later() { return 2.75; }
  static auto dependent() { return later(); }
};
int main() {
  auto function = &Operations<float>::apply;
  if (function(0.5f) != 0.75f) return 1;
  if (Operations<double>::apply(0.5) != 1.0) return 2;
  if (Operations<int>::apply(2) != 12) return 3;
  if (forward(0.5f) != 0.75f || forward(0.5) != 1.0) return 4;
  if (Operations<float>::forward(0.5f) != 0.75f) return 5;
  if (Operations<int>::sum(2) != 6.5) return 6;
  if (Factory::product(1.25).value != 1.25) return 7;
  if (Factory::dependent() != 2.75) return 8;
  int output = 0;
  Factory::nothing(); Factory::store(&output);
  return output != 7;
}
