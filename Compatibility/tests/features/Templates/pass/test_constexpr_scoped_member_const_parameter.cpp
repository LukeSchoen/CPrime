struct Value { constexpr int get(int value) const; };
constexpr int Value::get(const int value) const { return value; }
static_assert(Value{}.get(3) == 3);
int main() { return Value{}.get(3) != 3; }
