struct Value {
  int member;
  constexpr int add(int amount) { member += amount; return member; }
  constexpr int get() const { return member; }
};
constexpr int run() {
  Value value{3};
  auto pointer = &Value::add;
  Value *address = &value;
  int first = (value.*pointer)(4);
  return first + (address->*pointer)(2);
}
constexpr Value fixed{5};
constexpr int read() {
  auto pointer = &Value::get;
  return (fixed.*pointer)();
}
static_assert(run() == 16);
static_assert(read() == 5);
int main() { return run() != 16 || read() != 5; }
