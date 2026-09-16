struct Value { int prefix; int number; };
constexpr Value value{3, 9};
constexpr const int *address(const Value &object) {
    const int *pointer = &object.number;
    return pointer;
}
constexpr int read(const Value &object) {
    const int *pointer = &object.number;
    const int *alias{pointer};
    return *alias;
}
constexpr bool empty() { const int *pointer = nullptr; return pointer == nullptr; }
static_assert(address(value) == &value.number);
static_assert(read(value) == 9);
static_assert(read(Value{3, 7}) == 7);
static_assert(empty());
int main() { return address(value) != &value.number || read(value) != 9; }
