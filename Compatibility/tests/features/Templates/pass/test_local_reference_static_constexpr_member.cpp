struct Value { int number; };
constexpr Value value{9};
struct Outer { int prefix; Value member; };
constexpr Outer outer{3, {7}};
Value mutable_value{2};
int main() {
    const Value &reference = value;
    const Value &nested = outer.member;
    const Value &alias = reference;
    static_assert(reference.number == 9);
    static_assert(nested.number == 7);
    static_assert(alias.number == 9);
    Value &writable = mutable_value;
    Value &writable_alias = writable;
    writable_alias.number = 6;
    return reference.number != 9 || nested.number != 7 || alias.number != 9
        || mutable_value.number != 6 || &writable_alias != &mutable_value;
}
