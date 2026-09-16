#include <algorithm>
struct Iterator {
  int *p;
  int &operator*() const { return *p; }
  Iterator &operator++() { ++p; return *this; }
  bool operator!=(Iterator b) const { return p != b.p; }
};
int main() {
  int values[4] = { 1, 2, 3, 4 };
  std::fill(values + 1, values + 3, 7);
  if (values[0] != 1 || values[1] != 7 || values[2] != 7 || values[3] != 4) return 1;
  Iterator first = { values }, last = { values + 4 };
  std::fill(first, last, 9);
  for (int i = 0; i != 4; ++i) if (values[i] != 9) return 2;
  return 0;
}
