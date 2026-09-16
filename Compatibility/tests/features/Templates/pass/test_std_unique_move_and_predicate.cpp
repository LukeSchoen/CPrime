#include <algorithm>
static int live, moves, comparisons;
struct Value {
  int key;
  const Value *self;
  explicit Value(int n) : key(n), self(this) { ++live; }
  Value(const Value &) = delete;
  Value &operator=(const Value &) = delete;
  Value &operator=(Value &&other) {
    key = other.key;
    other.key = -1;
    ++moves;
    return *this;
  }
  bool operator==(const Value &other) const { ++comparisons; return key == other.key; }
  ~Value() { if (self == this) --live; }
};
struct Near {
  bool operator()(int a, int b) const { ++comparisons; return a / 10 == b / 10; }
};
int main() {
  {
    Value values[] = {Value(1),Value(1),Value(2),Value(2),Value(3),Value(1)};
    Value *end = std::unique(values,values+6);
    if (end != values+4 || comparisons != 5 || moves != 3) return 1;
    if (values[0].key != 1 || values[1].key != 2 || values[2].key != 3 || values[3].key != 1) return 2;
    if (live != 6) return 3;
    if (std::unique(values,values) != values || std::unique(values,values+1) != values+1) return 4;
  }
  if (live) return 5;
  int input[] = {11,12,25,29,31,39};
  comparisons = 0;
  int *end = std::unique(input,input+6,Near{});
  return end != input+3 || input[0] != 11 || input[1] != 25 || input[2] != 31 || comparisons != 5 ? 6 : 0;
}
