struct Value { int number; };
constexpr Value value{9};
constexpr const Value &object() { return value; }
constexpr const int &number() { return value.number; }
constexpr const int &member(const Value &object) { return object.number; }
static_assert(object().number == 9);
static_assert(number() == 9);
static_assert(&number() == &value.number);
static_assert(member(Value{4}) == 4);
int main() { return &object() != &value || number() != 9; }
