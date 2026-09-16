constexpr void change(int &value) { value += 3; return; }
constexpr void implicit(int &value) { value *= 2; }
constexpr void forward(int &value) { return change(value); }
constexpr int run() { int value = 4; change(value); implicit(value); forward(value); return value; }
static_assert(run() == 17);
int main() { return run() != 17; }
