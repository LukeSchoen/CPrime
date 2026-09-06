#include <iterator>
#include <type_traits>
struct Iter : std::iterator<std::bidirectional_iterator_tag, int> {
  int* p;
  Iter(int* v) : p(v) {}
  Iter& operator++() { ++p; return *this; }
  Iter& operator--() { --p; return *this; }
  bool operator!=(const Iter& rhs) const { return p != rhs.p; }
};
static_assert(std::is_same<std::iterator_traits<const int*>::value_type, int>::value, "value type");
int main() {
  int a[4] = {1, 2, 3, 4};
  Iter first(a), last(a + 4);
  if (std::distance(first, last) != 4) return 1;
  std::advance(last, -2);
  if (last.p != a + 2) return 2;
  if (std::next(first, 3).p != a + 3) return 3;
  return std::prev(a + 4, 2) != a + 2;
}
