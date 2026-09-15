struct Value { int member; };
constexpr Value source{4};
constexpr int change(Value value) { value.member += 3; return value.member; }
static_assert(change(source) == 7);
static_assert(source.member == 4);
int main() { return change(source) != 7 || source.member != 4; }
