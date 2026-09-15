constexpr int run() { int i = 0; int checks = 0; int total = 0; for (i = 1, total = 2; ++checks, i < 4; ++i, ++total) {} return checks * 10 + total; }
constexpr int conditions() { int value = 0; if (++value, true) value += 2; while (++value, value < 5) {} return ++value, value; }
static_assert(run() == 45);
static_assert(conditions() == 6);
int main() { return run() != 45 || conditions() != 6; }
