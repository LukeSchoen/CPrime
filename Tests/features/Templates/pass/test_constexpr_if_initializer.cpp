constexpr int run(int input) { int value = 9; if (int value = input + 1; value > 2) return value; else value += 3; return value; }
constexpr int expressions() { int value = 0; if (++value; value == 1) ++value; if (; value == 2) ++value; return value; }
static_assert(run(3) == 4);
static_assert(run(0) == 9);
static_assert(expressions() == 3);
int main() { return run(3) != 4 || run(0) != 9 || expressions() != 3; }
