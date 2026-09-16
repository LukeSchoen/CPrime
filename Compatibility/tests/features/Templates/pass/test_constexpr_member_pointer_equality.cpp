struct Value {
  int first;
  int second;
  constexpr int get() const { return 1; }
  constexpr int other() const { return 2; }
};
constexpr bool run() {
  int Value::*first = &Value::first;
  int Value::*same = &Value::first;
  int Value::*second = &Value::second;
  int Value::*empty = nullptr;
  return first == same && first != second && first != nullptr
      && empty == nullptr;
}
struct Padding { int padding; };
struct Derived : Padding, Value {};
constexpr bool functions() {
  auto first = &Value::get;
  auto same = &Value::get;
  auto other = &Value::other;
  int (Derived::*converted)() const = first;
  int (Derived::*empty)() const = nullptr;
  return first == same && first != other && first == converted
      && nullptr == empty && converted != empty;
}
static_assert(run());
static_assert(functions());
int main() { return !run() || !functions(); }
