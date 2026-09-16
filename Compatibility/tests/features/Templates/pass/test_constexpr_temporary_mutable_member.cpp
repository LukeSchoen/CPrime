struct Value { mutable int number; };
static_assert(Value{9}.number == 9);
constexpr int read(const Value &value) { return value.number; }
static_assert(read(Value{4}) == 4);
struct Outer { mutable Value inner; mutable int numbers[2]; };
static_assert(Outer{{7}, {3, 8}}.inner.number == 7);
static_assert(Outer{{7}, {3, 8}}.numbers[1] == 8);
static_assert(Outer{}.numbers[1] == 0);
constexpr int initialized = read(Value{6});
static_assert(initialized == 6);
struct Condition {
    mutable int value;
    constexpr explicit operator bool() const { return value != 0; }
};
int main() {
    if constexpr (Condition{0}) return 1;
    if constexpr (Condition{3}) return 0;
    return 2;
}
