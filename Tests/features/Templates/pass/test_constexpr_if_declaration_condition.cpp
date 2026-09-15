constexpr int run(int input) {
  if (int value = input) return value + 1;
  else return value + 3;
}
constexpr int scoped(int input) {
  int value = 19;
  if (int seed = input + 1; int value{seed - 2}) return ++value;
  else value += seed;
  return value;
}
constexpr int reference(int input) {
  if (int &value = input) value += 2;
  else value = 4;
  return input;
}
static_assert(run(4) == 5);
static_assert(run(0) == 3);
static_assert(scoped(4) == 4);
static_assert(scoped(1) == 19);
static_assert(reference(1) == 3);
static_assert(reference(0) == 4);
int main() {
  volatile int zero = 0, one = 1, four = 4;
  return run(four) != 5 || run(zero) != 3
      || scoped(four) != 4 || scoped(one) != 19
      || reference(one) != 3 || reference(zero) != 4;
}
