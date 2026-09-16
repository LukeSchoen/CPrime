constexpr int sum() { int result = 0; for (int i = 0; i < 4; ++i) { if (i == 1) continue; result += i; } return result; }
constexpr int scope() { int i = 9; int count = 0; for (int i = 0; i < 2; ++i) ++count; return i + count; }
constexpr int empty() { int count = 0; for (;;) { if (++count == 3) break; } return count; }
constexpr int step() { int count = 0; int increments = 0; for (;count < 5; ++increments) { ++count; if (count == 2) continue; if (count == 3) break; } return count * 10 + increments; }
static_assert(sum() == 5);
static_assert(scope() == 11);
static_assert(empty() == 3);
static_assert(step() == 32);
int main() { return sum() != 5 || scope() != 11 || empty() != 3 || step() != 32; }
