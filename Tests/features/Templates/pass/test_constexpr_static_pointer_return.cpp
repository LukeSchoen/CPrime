struct Value { int prefix; int number; };
constexpr Value value{4, 9};
constexpr const int *address() { return &value.number; }
constexpr const int *member(const Value &object) { return &object.number; }
constexpr const int *stored = address();
static_assert(address() == &value.number);
static_assert(*address() == 9);
static_assert(member(value) == stored);
int main() { return *address() != 9; }
