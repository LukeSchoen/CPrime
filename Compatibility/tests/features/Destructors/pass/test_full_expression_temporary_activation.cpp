// EXPECT_EXIT: 0
int alive, destroyed, order;
struct Value {
  int id;
  Value(int n) : id(n) { ++alive; }
  Value(const Value &other) : id(other.id) { ++alive; }
  ~Value() { --alive; ++destroyed; order = order * 10 + id; }
  int get() const { return id; }
};
int observe(const Value &a, const Value &b) {
  return alive == 2 && a.id == 1 && b.id == 2;
}
int condition(bool flag) { return flag && Value(3).get(); }
int select(bool flag) { return flag ? Value(4).get() : Value(5).get(); }
int return_value() { return Value(6).get(); }
int main() {
  if (!observe(Value(1), Value(2))) return 1;
  if (alive != 0 || destroyed != 2 || order != 21) return 2;
  if (condition(false) || alive != 0 || destroyed != 2) return 3;
  if (!condition(true) || alive != 0 || destroyed != 3) return 4;
  if (select(false) != 5 || alive != 0 || destroyed != 4) return 5;
  int selected = select(true);
  if (selected != 4 || alive != 0 || destroyed != 5) return 6;
  if (return_value() != 6 || alive != 0 || destroyed != 6) return 7;
  { const Value &ref = Value(7); if (alive != 1 || ref.id != 7) return 8; }
  if (alive != 0 || destroyed != 7) return 9;
  for (int i=0; i<3; ++i) { (void)Value(8); if (alive) return 10; }
  return destroyed != 10;
}
