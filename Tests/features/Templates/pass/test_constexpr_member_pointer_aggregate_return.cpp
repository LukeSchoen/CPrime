struct Result { int values[3]; };
struct Value {
  int member;
  constexpr Result make(int amount) const {
    return Result{{member, amount, member + amount}};
  }
};
struct Padding { int padding; };
struct Derived : Padding, Value {};
constexpr int run() {
  Derived value{{19}, {3}};
  auto pointer = &Value::make;
  Result (Derived::*converted)(int) const = pointer;
  Result result = (value.*pointer)(4);
  Result adjusted = (value.*converted)(5);
  return result.values[2] + adjusted.values[2];
}
static_assert(run() == 15);
int main() { return run() != 15; }
