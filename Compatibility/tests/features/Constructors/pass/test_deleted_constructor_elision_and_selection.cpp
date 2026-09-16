#include <type_traits>
struct Value {
    int number;
    Value() = delete;
    Value(int value) : number(value) {}
    Value(const Value&) = delete;
    Value(Value&&) = delete;
};
Value make() { return Value(7); }
struct Private { private: Private(int); };
struct Converts { explicit operator int() const { return 5; } };
static_assert(!std::is_constructible<Private, int>::value, "access from unrelated context");
static_assert(std::is_constructible<int, Converts>::value, "direct explicit conversion");
static_assert(!std::is_default_constructible<Value>::value, "deleted default constructor");
static_assert(!std::is_copy_constructible<Value>::value, "deleted copy constructor");
static_assert(!std::is_move_constructible<Value>::value, "deleted move constructor");
static_assert(std::is_constructible_v<Value, int>, "select a nondeleted overload");
static_assert(std::is_default_constructible_v<const int>, "value initialization");
static_assert(!std::is_default_constructible_v<void>, "void is not an object");
static_assert(!std::is_copy_constructible_v<void>, "void cannot be copied");
static_assert(!std::is_move_constructible_v<void>, "void cannot be moved");
static_assert(!std::is_default_constructible_v<int&>, "reference needs initializer");
static_assert(!std::is_constructible_v<int, int*>, "invalid pointer conversion");
static_assert(std::is_same_v<std::add_lvalue_reference_t<void>, void>, "nonreferenceable type");
int main() { Value value = make(); return value.number != 7; }
