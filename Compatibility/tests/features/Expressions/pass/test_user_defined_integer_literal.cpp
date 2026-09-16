constexpr unsigned long long operator "" _twice(unsigned long long value) {
    return value * 2;
}
static_assert(21_twice == 42);
int main() { return 21_twice != 42; }