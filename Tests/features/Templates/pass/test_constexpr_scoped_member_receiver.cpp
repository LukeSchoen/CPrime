struct Value { int member; constexpr int get() const; constexpr int add(int amount); };
constexpr int Value::get() const { return member; }
constexpr int Value::add(int amount) { this->member += amount; return member; }
constexpr int run() { Value value{4}; int result = value.add(3); return result + value.get(); }
static_assert(Value{5}.get() == 5);
static_assert(run() == 14);
int main() { return run() != 14; }
