constexpr int choose(int value) { if (value > 0) return value + 1; return 0; }
static_assert(choose(3) == 4);
static_assert(choose(-1) == 0);
int main() { return choose(3) != 4 || choose(-1) != 0; }
