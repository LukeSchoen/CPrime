struct Value {
  int kind;
  Value() : kind(0) {}
  explicit Value(Value &) : kind(1) {}
  template<class T> Value(T &) : kind(2) {}
};
int main() {
  Value value;
  auto direct = [value]() { return value.kind; };
  auto copied = [value = value]() { return value.kind; };
  return direct() != 1 || copied() != 2;
}
