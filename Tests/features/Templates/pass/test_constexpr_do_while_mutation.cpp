int runtime_condition();
constexpr int run(int count) { int total = 0; do { ++total; --count; if (count == 1) continue; if (count == 0) break; } while (count > 0); return total; }
constexpr int checks() { int count = 0; int tests = 0; do { ++count; if (count < 3) continue; break; } while (++tests < 5); return count * 10 + tests; }
constexpr int early() { do { return 7; } while (runtime_condition()); }
static_assert(run(0) == 1);
static_assert(run(3) == 3);
static_assert(checks() == 32);
static_assert(early() == 7);
int main() { return run(0) != 1 || run(3) != 3 || checks() != 32 || early() != 7; }
