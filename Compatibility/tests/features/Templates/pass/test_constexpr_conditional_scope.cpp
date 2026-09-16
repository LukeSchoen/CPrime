int side_effect();
constexpr int run(int input) {
  int value = 3;
  if (input > 0) { int value = 7; if (input == 1) return value; }
  else { value = 4; }
  if (false) side_effect();
  return value;
}
static_assert(run(1) == 7);
static_assert(run(2) == 3);
static_assert(run(0) == 4);
int main() { return run(1) != 7 || run(2) != 3 || run(0) != 4; }
