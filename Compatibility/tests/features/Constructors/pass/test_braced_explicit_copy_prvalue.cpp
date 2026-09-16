int copies;
struct Value {
  int kind;
  Value() : kind(0) {}
  explicit Value(const Value &) : kind(3) { ++copies; }
};
struct Holder { Value value; };
int main() {
  Value source;
  Value copy = Value{source};
  Holder holder{Value{source}};
  Value parens(Value{});
  Value braces{Value{}};
  return copy.kind != 3 || holder.value.kind != 3 || copies != 2
      || parens.kind != 0 || braces.kind != 0;
}
