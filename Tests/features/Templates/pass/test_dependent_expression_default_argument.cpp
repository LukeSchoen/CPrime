#include <utility>
struct Number { int value; };
int extract(int value) { return value; }
int extract(int *value) { return *value; }
template<class T, typename = decltype(extract(std::declval<T>()))>
int operator+(const Number &left, T right) { return left.value + extract(right); }
int add_pointer(const Number &left, int *value) {
  using Pointer = int *;
  Pointer p = value;
  return left + p;
}
int main() {
  Number n;
  n.value = 19;
  int value = 23;
  if (n + value != 42) return 1;
  if (add_pointer(n, &value) != 42) return 2;
  return 0;
}
