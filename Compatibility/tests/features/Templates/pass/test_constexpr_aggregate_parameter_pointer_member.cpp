struct Value { int member; int *pointer; };
constexpr int change(Value value) { value.member = 9; *value.pointer = 7; return value.member; }
constexpr int run() { int target = 3; Value value{4, &target}; int result = change(value); return result + value.member + target; }
static_assert(run() == 20);
int main() { return run() != 20; }
