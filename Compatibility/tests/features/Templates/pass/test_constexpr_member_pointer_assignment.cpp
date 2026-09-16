struct Value {
  int first; int second;
  constexpr int get() const { return first; }
  constexpr int other() const { return second; }
};
struct Padding { int padding; };
struct Derived : Padding, Value {};
constexpr bool run() {
  int Value::*pointer = &Value::first;
  pointer = &Value::second;
  bool changed = pointer == &Value::second;
  pointer = nullptr;
  return changed && !pointer;
}
static_assert(run());
constexpr bool functions() {
  int (Derived::*pointer)() const = &Value::get;
  pointer = &Value::other;
  Derived value{{3}, {4, 9}};
  bool changed = (value.*pointer)() == 9;
  pointer = pointer;
  changed = changed && pointer == &Value::other;
  int (Derived::*copy)() const = nullptr;
  copy = pointer = &Value::get;
  changed = changed && (value.*copy)() == 4;
  pointer = nullptr;
  return changed && !pointer;
}
static_assert(functions());
int main() { return !(run() && functions()); }
