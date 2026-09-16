constexpr int run() {
  int values[] = {1, 2, 3};
  int sum = 0;
  for (auto &value : values) {
    value += 1;
    sum += value;
  }
  return sum;
}
constexpr int copied() {
  int values[] = {1, 2, 3, 4};
  int sum = 0;
  for (auto value : values) {
    if (value == 2) continue;
    if (value == 4) break;
    sum += ++value;
  }
  return sum + values[0];
}
static_assert(run() == 9);
static_assert(copied() == 7);
int main() { return run() != 9 || copied() != 7; }
