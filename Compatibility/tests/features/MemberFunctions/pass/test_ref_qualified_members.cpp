struct Value {
  int pick() & { return 1; }
  int pick() const& { return 2; }
  int pick() && { return 3; }
  int operator~() const&;
  int operator~() &&;
};
int Value::operator~() const& { return 4; }
int Value::operator~() && { return 5; }
int main() {
  Value v;
  const Value c;
  if (v.pick() != 1 || c.pick() != 2) return 1;
  if (Value().pick() != 3) return 2;
  if (static_cast<Value&&>(v).pick() != 3) return 3;
  if (~v != 4 || ~Value() != 5) return 4;
  return 0;
}
