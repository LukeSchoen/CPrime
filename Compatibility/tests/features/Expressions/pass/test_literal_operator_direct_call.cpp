constexpr unsigned long long operator "" _twice(unsigned long long value) { return value * 2; }
static_assert(operator "" _twice(21) == 42);
int main() { return operator "" _twice(21) != 42; }