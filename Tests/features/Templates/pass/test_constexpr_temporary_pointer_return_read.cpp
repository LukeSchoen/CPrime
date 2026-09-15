struct Value { int prefix; int number; };
constexpr const int *address(const Value &value) { return &value.number; }
static_assert(*address(Value{3, 9}) == 9);
static_assert(*address(Value{7, 4}) == 4);
int main() {}
