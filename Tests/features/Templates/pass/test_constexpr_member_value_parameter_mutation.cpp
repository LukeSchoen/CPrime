struct Value { constexpr int change(int value) const { value += 3; return value; } };
static_assert(Value{}.change(4) == 7);
int main() { return Value{}.change(4) != 7; }
