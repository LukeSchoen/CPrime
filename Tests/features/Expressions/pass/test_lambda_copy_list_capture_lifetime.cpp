#include <initializer_list>
int alive, copies;
struct Element {
  int value;
  Element(int n) : value(n) { ++alive; }
  Element(const Element &other) : value(other.value) { ++alive; ++copies; }
  ~Element() { --alive; }
};
int exercise() {
  auto closure = [values = {Element(3), Element(7)}] {
    return values.begin()[0].value + values.begin()[1].value;
  };
  if (alive != 2 || copies != 0) return 1;
  return closure() != 10;
}
int main() {
  int result = exercise();
  if (result || alive) return 1;
  int immediate = [values = {Element(2), Element(5)}] {
    return values.begin()[0].value + values.begin()[1].value;
  }();
  return immediate != 7 || alive != 0 || copies != 0;
}
