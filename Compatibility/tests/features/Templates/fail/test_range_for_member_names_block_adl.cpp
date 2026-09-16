// EXPECT_COMPILE_FAIL: 1
namespace ranges {
struct Range { enum { begin = 1, end = 2 }; int values[2]; };
int *begin(Range &range) { return range.values; }
int *end(Range &range) { return range.values + 2; }
}
int main() {
  ranges::Range range{{1, 2}};
  for (int value : range) (void)value;
}
