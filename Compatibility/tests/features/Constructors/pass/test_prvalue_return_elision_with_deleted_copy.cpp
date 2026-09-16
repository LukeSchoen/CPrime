// EXPECT_COMPILE_ARGS: -std=c++17
// Regression: `return T(args);` and `return {args};` initialize the caller's
// result object directly, so a deleted copy or move constructor must not be
// required.  Treating the result type as a base of itself used to strip the
// caller-provided slot and force a move construction.
struct Value {
  int number;
  Value() = delete;
  Value(int value) : number(value) {}
  Value(const Value&) = delete;
  Value(Value&&) = delete;
};

Value make_value() { return Value(7); }

template<class Seed> Value make_seeded(Seed seed) { return Value(seed); }

struct Pair {
  int first;
  int second;
  Pair(int left, int right) : first(left), second(right) {}
  Pair(const Pair&) = delete;
  Pair(Pair&&) = delete;
};

Pair make_pair_value() { return {1, 2}; }

int main() {
  Value value = make_value();
  if (value.number != 7) return 1;
  Value seeded = make_seeded(8);
  if (seeded.number != 8) return 2;
  Pair pair = make_pair_value();
  return pair.first == 1 && pair.second == 2 ? 0 : 3;
}
