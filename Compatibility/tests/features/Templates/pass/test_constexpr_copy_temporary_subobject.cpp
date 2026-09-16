struct Value { int number; };
struct Outer { int prefix; mutable Value member; };
struct Mutable { mutable int number; };
struct Holder { Mutable member; };
int main() {
    constexpr Value copy = Outer{3, {9}}.member;
    constexpr Mutable mutable_copy = Holder{{7}}.member;
    static_assert(copy.number == 9);
    return copy.number != 9 || mutable_copy.number != 7;
}
