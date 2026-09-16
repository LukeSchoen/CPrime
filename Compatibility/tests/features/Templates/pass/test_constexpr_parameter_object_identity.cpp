constexpr int update(int value) { int *pointer = &value; (*pointer)++; return value; }
constexpr int change(int *pointer, int *other) { pointer = other; *pointer = 9; return *pointer; }
constexpr int run() { int a = 3; int b = 4; int *pointer = &a; int result = change(pointer, &b); return result + a + b + update(a) + (pointer == &a); }
static_assert(run() == 26);
int main() { return run() != 26; }
