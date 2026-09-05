// EXPECT_EXIT: 0
int alive, destroyed;
struct Value {
  int value;
  Value(int n) : value(n) { ++alive; }
  Value(const Value &) = delete;
  Value(Value &&) = delete;
  ~Value() { --alive; ++destroyed; }
};
int consume(Value value) { return value.value + alive; }
int main() {
  if (consume(Value(7)) != 8) return 1;
  return alive != 0 || destroyed != 1;
}
