int alive, copies, moves;
struct Value {
  int value;
  Value(int n) : value(n) { ++alive; }
  Value(const Value &other) : value(other.value) { ++alive; ++copies; }
  Value(Value &&other) : value(other.value) { other.value = -1; ++alive; ++moves; }
  ~Value() { --alive; }
};
Value make_value() { return Value(11); }
int exercise() {
  Value source(7);
  {
    auto copied = [object = source]() { return object.value; };
    auto moved = [object = static_cast<Value &&>(source)]() { return object.value; };
    auto direct = [object = make_value()]() { return object.value; };
    auto parens = [object(make_value())]() { return object.value; };
    auto braces = [object{make_value()}]() { return object.value; };
    if (copies != 1 || moves != 1 || alive != 6) return 1;
    if (copied() != 7 || moved() != 7 || direct() != 11 || source.value != -1
        || parens() != 11 || braces() != 11) return 2;
  }
  return alive != 1;
}
int main() { int result = exercise(); return result ? result : alive; }
