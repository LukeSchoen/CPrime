constexpr int update(int value) { value += 3; return value; }
constexpr int run() { int value = 4; int result = update(value); return result + value; }
static_assert(run() == 11);
int main() { return run() != 11; }
