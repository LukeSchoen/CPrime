struct Value { int member; constexpr int get() const { return 7; } };
constexpr bool present() {
  auto pointer = &Value::get;
  return bool(pointer);
}
constexpr bool absent() {
  int (Value::*pointer)() const = nullptr;
  return !pointer;
}
constexpr int data() {
  int Value::*present = &Value::member;
  int Value::*absent{};
  if (present && !absent) return 1;
  return 0;
}
struct Padding { int padding; };
struct Derived : Padding, Value {};
constexpr bool converted_null() {
  int (Value::*source)() const = nullptr;
  int (Derived::*converted)() const = source;
  return !converted;
}
static_assert(present());
static_assert(absent());
static_assert(data() == 1);
static_assert(converted_null());
int main() { return !present() || !absent() || data() != 1 || !converted_null(); }
