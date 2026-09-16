int copies, alive;
struct Value {
  int number;
  Value(int n) : number(n) { ++alive; }
  Value(const Value &other) : number(other.number) { ++copies; ++alive; }
  ~Value() { --alive; }
  auto snapshot() { return [*this]() mutable { return ++number; }; }
};
int main() {
  {
    Value value(4);
    auto closure = value.snapshot();
    value.number = 20;
    if (copies != 1 || alive != 2 || closure() != 5 || closure() != 6
        || value.number != 20) return 1;
  }
  return alive;
}
