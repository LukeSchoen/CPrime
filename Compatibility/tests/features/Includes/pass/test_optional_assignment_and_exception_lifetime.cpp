#include <optional>
int live, copies, assignments;
struct Value {
  int number;
  Value(int n) : number(n) { if (n == 99) throw n; ++live; }
  Value(const Value &v) : number(v.number) { ++live; ++copies; }
  Value &operator=(const Value &v) { number = v.number; ++assignments; return *this; }
  ~Value() { --live; }
};
int main() {
  {
    std::optional<Value> a;
    a.emplace(7);
    std::optional<Value> b(a);
    if (live != 2 || copies != 1 || b->number != 7) return 1;
    b = a;
    if (live != 2 || assignments != 1) return 2;
    a.reset();
    a = b;
    if (live != 2 || copies != 2) return 3;
    try { a.emplace(99); return 4; } catch (int n) { if (n != 99) return 5; }
    if (a.has_value() || live != 1) return 6;
    try { a.value(); return 7; } catch (const std::bad_optional_access &) {}
    b = a;
    if (b.has_value() || live) return 8;
    b.emplace(8);
  }
  return live;
}
