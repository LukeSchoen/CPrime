struct Value { int member; };
constexpr const Value &selected(const Value &value, int &calls) {
  ++calls;
  return value;
}
constexpr int run() {
  Value source{4};
  int calls = 0;
  Value copy = selected(source, calls);
  copy.member += 3;
  return source.member * 10 + copy.member + calls * 100;
}
static_assert(run() == 147);
int main() { return run() != 147; }
