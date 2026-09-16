constexpr int sum(int input) {
  int result = 0;
  while (int value = input--) {
    if (value == 2) continue;
    result += value;
  }
  return result;
}
constexpr int counted(int input) {
  int result = 0;
  for (int index = 0; int value{input - index}; ++index, result += value) {
    if (value == 2) continue;
    if (value == 1) break;
    result += 10;
  }
  return result;
}
constexpr int converted(int input) {
  int result = 0;
  while (int(input--)) ++result;
  if (bool(result)) return result;
  return 0;
}
static_assert(sum(4) == 8);
static_assert(sum(0) == 0);
static_assert(counted(4) == 29);
static_assert(converted(4) == 4);
int main() {
  volatile int input = 4;
  return sum(input) != 8 || counted(input) != 29 || converted(input) != 4;
}
