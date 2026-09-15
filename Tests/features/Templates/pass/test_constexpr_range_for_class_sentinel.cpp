struct Sentinel { int *limit; };
struct Iterator {
  int *position;
  constexpr int &operator*() const { return *position; }
  constexpr Iterator &operator++() { ++position; return *this; }
};
constexpr bool operator!=(const Iterator &iterator, const Sentinel &sentinel) {
  return iterator.position != sentinel.limit;
}
struct Range {
  int values[3];
  constexpr Iterator begin() { return Iterator{values}; }
  constexpr Sentinel end() { return Sentinel{values + 3}; }
};
constexpr int run() {
  Range range{{1, 2, 3}};
  int sum = 0;
  for (int &value : range) sum += ++value;
  return sum;
}
static_assert(run() == 9);
int main() { return run() != 9; }
