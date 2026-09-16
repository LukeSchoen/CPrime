constexpr int change(int &value) { value += 3; return value; }
constexpr int run() { int value = 4; int result = change(value); return result + value; }
static_assert(run() == 14);
int main() { return run() != 14; }
