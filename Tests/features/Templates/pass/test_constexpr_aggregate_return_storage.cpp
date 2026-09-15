struct Value { int fields[2]; };
constexpr Value make() { return Value{{3, 4}}; }
constexpr Value named() { Value value{{5, 6}}; return value; }
constexpr int run() {
  Value first = make();
  Value second = named();
  first.fields[0] += 7;
  return first.fields[0] + second.fields[0];
}
static_assert(run() == 15);
int main() { return run() != 15; }
