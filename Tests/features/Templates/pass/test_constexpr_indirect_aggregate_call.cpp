struct Value { int fields[3]; };
constexpr Value make(int input) { return Value{{input, input + 1, input + 2}}; }
constexpr Value other(int input) { return Value{{input, input + 2, input + 4}}; }
constexpr Value invoke(Value (*pointer)(int), int input) { return pointer(input); }
constexpr int run(bool change) {
  auto pointer = &make;
  if (change) pointer = &other;
  Value value = invoke(pointer, 4);
  return value.fields[2];
}
static_assert(run(false) == 6);
static_assert(run(true) == 8);
constexpr int loop() {
  auto pointer = &make;
  for (int index = 0; index < 1; pointer = &other) ++index;
  return pointer(2).fields[2];
}
static_assert(loop() == 6);
int main() { return run(false) != 6 || run(true) != 8 || loop() != 6; }
