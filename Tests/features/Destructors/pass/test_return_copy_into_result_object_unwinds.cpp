// EXPECT_EXIT: 0
// The result object of a return statement is built in the caller's storage but
// belongs to this frame until the return completes, so a later cleanup that
// throws must unwind through it.
int live, destroyed;
struct Value {
  int id;
  Value(int n) : id(n) { ++live; }
  Value(const Value &other) : id(other.id) { ++live; }
  ~Value() noexcept(false) {
    --live;
    ++destroyed;
    if (id == 1) throw 1;
  }
};
Value build(bool flag) {
  Value throwing(1), first(2), second(3);
  return flag ? first : second;
}
int main() {
  try { build(true); } catch (...) {}
  if (live != 0) return 1;
  if (destroyed != 4) return 2;
  return 0;
}
