struct Value { int fields[3]; };
struct Small { int value; };
struct Pointer { int *value; };
constexpr Small small(int value) { return Small{value}; }
constexpr Pointer refer(int &value) { return Pointer{&value}; }
constexpr Value make(int input) { return Value{{input, input + 1, input + 2}}; }
constexpr Value operator+(const Value &value, int amount) {
  return Value{{value.fields[0] + amount, value.fields[1], value.fields[2]}};
}
constexpr int run() {
  Value first = make(3);
  Value second = make(7);
  Value third = first + 2;
  int value = 1;
  Pointer pointer = refer(value);
  *pointer.value += 2;
  return first.fields[2] + second.fields[0] + third.fields[0] + small(value).value;
}
static_assert(run() == 20);
int main() { return run() != 20; }
