// EXPECT_EXIT: 0
int alive, destroyed;
struct Value {
  int n;
  const Value *self;
  Value(int input) : n(input), self(this) { ++alive; }
  Value(const Value&) = delete;
  Value(Value&&) = delete;
  ~Value() { --alive; ++destroyed; }
};
int main() {
  auto make = [](int n) { return Value(n); };
  { Value value = make(7); if(value.n!=7 || value.self!=&value || alive!=1) return 1; }
  if(alive || destroyed!=1) return 2;
  auto nested = [](const Value &input) { return Value(input.n + 1); };
  { Value value = nested(make(8)); if(value.n!=9 || value.self!=&value || alive!=1 || destroyed!=2) return 3; }
  return alive || destroyed!=3;
}
