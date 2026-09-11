// EXPECT_EXIT: 0
// A by-value class argument is the caller's object: it is the temporary the
// caller built, and the caller destroys it when the full expression ends.
int live, destroyed, inside;
struct Value {
  Value() { ++live; }
  Value(const Value &) { ++live; }
  ~Value() { --live; ++destroyed; }
};
void consume(Value value, int) { inside = live; }
int main() {
  int seen = -1;
  consume(Value(), 0), seen = destroyed;
  if (inside != 1) return 1;
  if (seen != 0) return 2;
  if (destroyed != 1 || live != 0) return 3;
  return 0;
}
