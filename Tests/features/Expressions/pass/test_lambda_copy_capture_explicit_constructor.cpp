int copies;
struct Value {
  int value;
  Value(int n) : value(n) {}
  explicit Value(const Value &other) : value(other.value) { ++copies; }
};
int main() {
  Value value(7);
  auto closure = [value]() { return value.value; };
  return closure() != 7 || copies != 1;
}
