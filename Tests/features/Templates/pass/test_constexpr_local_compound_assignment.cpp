constexpr int update() {
  int value = 3;
  int &alias = value;
  int *pointer = &value;
  value += 4;
  alias *= 3;
  *pointer -= 1;
  value /= 2;
  value %= 6;
  value <<= 2;
  value >>= 1;
  value |= 3;
  value &= 10;
  value ^= 6;
  return value;
}
static_assert(update() == 12);
constexpr int chained() { int a = 1; int b = 2; a = b = 7; return a + b; }
static_assert(chained() == 14);
int main() { return update() != 12 || chained() != 14; }
