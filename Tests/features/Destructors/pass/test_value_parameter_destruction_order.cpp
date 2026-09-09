// EXPECT_EXIT: 0
int alive, failures;
struct Value {
  int index;
  Value() : index(alive++) {}
  Value(const Value&) : index(alive++) {}
  ~Value() { if (--alive != index) ++failures; }
};
void consume(Value, int, Value) { Value local; }
void unwind(Value, int, Value) { Value local; throw 7; }
int main() {
  {
    Value source;
    consume(source, 0, source);
    if (alive != 1 || failures) return 1;
    try { unwind(source, 0, source); }
    catch (int value) { if (value != 7) return 2; }
    if (alive != 1 || failures) return 3;
  }
  return alive || failures;
}
