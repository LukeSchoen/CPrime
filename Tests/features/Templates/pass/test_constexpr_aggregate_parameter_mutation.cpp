struct Value { int member; };
constexpr int change(Value value) { value.member += 3; return value.member; }
constexpr int run() { Value value{4}; int result = change(value); return result + value.member; }
static_assert(run() == 11);
int main() { return run() != 11; }
