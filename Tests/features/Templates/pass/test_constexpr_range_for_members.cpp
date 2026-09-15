struct Range {
  int values[3];
  constexpr int *begin() { return values; }
  constexpr int *end() { return values + 3; }
};
constexpr Range &select(Range &range, int &calls) {
  ++calls;
  return range;
}
constexpr int run() {
  Range range{{1, 2, 3}};
  int calls = 0;
  int sum = 0;
  for (int &value : select(range, calls)) {
    value += 2;
    sum += value;
  }
  return sum + calls * 10 + range.values[0];
}
static_assert(run() == 25);
constexpr int temporary() {
  int sum = 0;
  for (int &value : Range{{2, 3, 4}}) sum += ++value;
  return sum;
}
static_assert(temporary() == 12);
int main() { return run() != 25 || temporary() != 12; }
