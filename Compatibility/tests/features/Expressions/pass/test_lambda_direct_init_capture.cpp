struct Value {
  int kind;
  Value() : kind(0) {}
  explicit Value(const Value &) : kind(3) {}
};
int main() {
  int value = 4;
  Value object;
  auto parens = [number(value + 1), copy(object), &alias(value)]() mutable {
    alias += number;
    return copy.kind;
  };
  auto braces = [number{value + 2}, copy{object}, &alias{value}] {
    return number + copy.kind + alias;
  };
  return parens() != 3 || value != 9 || braces() != 18;
}
