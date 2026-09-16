struct Value { constexpr const Value *self() const; };
constexpr const Value *Value::self() const { return this; }
constexpr Value source{};
static_assert(source.self() == &source);
int main() { return source.self() != &source; }
