constexpr int sum(int count) {
  int total = 0;
  while (count > 0) {
    int current = count;
    --count;
    if (current == 3) continue;
    if (current == 1) break;
    total += current;
  }
  return total;
}
constexpr int early(int count) { while (count > 0) { if (count == 2) return 9; --count; } return 0; }
static_assert(sum(4) == 6);
static_assert(sum(0) == 0);
static_assert(early(4) == 9);
int main() { return sum(4) != 6 || sum(0) != 0 || early(4) != 9; }
