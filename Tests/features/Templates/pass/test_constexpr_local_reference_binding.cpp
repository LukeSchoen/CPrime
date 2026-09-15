struct Value { int number; };
constexpr Value value{9};
constexpr int read(const Value &object) {
    const int &alias = object.number;
    return alias;
}
constexpr int temporary() {
    const Value &alias = Value{7};
    return alias.number;
}
constexpr const int &reference(const Value &object) {
    const int &first = object.number;
    const int &second = first;
    return second;
}
static_assert(read(value) == 9);
static_assert(read(Value{4}) == 4);
static_assert(temporary() == 7);
static_assert(&reference(value) == &value.number);
static_assert(reference(Value{3}) == 3);
int main() {
    return read(value) != 9 || temporary() != 7
        || &reference(value) != &value.number;
}
