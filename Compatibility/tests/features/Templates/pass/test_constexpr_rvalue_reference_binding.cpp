struct Value { int number; };
constexpr int value = 9;
constexpr int read() {
    const int &&alias = static_cast<const int &&>(value);
    return alias;
}
constexpr int temporary() {
    Value &&alias = Value{7};
    return alias.number;
}
static_assert(read() == 9);
static_assert(temporary() == 7);
int accept(Value &&object) { return object.number; }
int main() { return read() != 9 || temporary() != 7 || accept(Value{4}) != 4; }
