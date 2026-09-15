namespace ranges {
struct Range { enum { begin = 1 }; int values[2]; };
constexpr int *begin(Range &range) { return range.values; }
constexpr const int *end(Range &range) { return range.values + 2; }
}
int *begin(ranges::Range &) = delete;
const int *end(ranges::Range &) = delete;
constexpr int run() {
  ranges::Range range{{3, 4}};
  int sum = 0;
  for (int &value : range) sum += ++value;
  return sum;
}
static_assert(run() == 9);
int main() { return run() != 9; }
