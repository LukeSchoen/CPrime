struct Value { int number; };
constexpr int read(const Value &value) { return value.number; }
constexpr int forward(const Value &value) { return read(value); }
constexpr int combine(const Value &first, const Value &second) {
    return first.number * 10 + second.number;
}
static_assert(read(Value{4}) == 4);
static_assert(read(Value{9}) == 9);
static_assert(forward(Value{7}) == 7);
static_assert(combine(Value{4}, Value{9}) == 49);
constexpr Value named{6};
static_assert(read(named) == 6);
int main() {}
