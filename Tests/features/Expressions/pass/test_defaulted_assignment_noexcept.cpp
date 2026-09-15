static int copies, moves;
struct Value {
    int number;
    Value& operator=(const Value& other) noexcept { ++copies; number = other.number; return *this; }
    Value& operator=(Value&& other) noexcept(false) { ++moves; number = other.number; other.number = 0; return *this; }
};
struct Owner {
    Value value;
    Owner& operator=(const Owner&) = default;
    Owner& operator=(Owner&&) = default;
};
struct Explicit {
    Value value;
    Explicit& operator=(const Explicit&) noexcept(false) = default;
    Explicit& operator=(Explicit&&) noexcept = default;
};
struct ArrayOwner {
    Value values[2];
    ArrayOwner& operator=(const ArrayOwner&) = default;
    ArrayOwner& operator=(ArrayOwner&&) = default;
};
int main() {
    Owner first = {{1}}, second = {{2}};
    Explicit third = {{3}}, fourth = {{4}};
    ArrayOwner array_first, array_second;
    static_assert(noexcept(first = second), "inferred copy assignment");
    static_assert(!noexcept(first = static_cast<Owner&&>(second)), "inferred move assignment");
    static_assert(!noexcept(third = fourth), "explicit throwing copy assignment");
    static_assert(noexcept(third = static_cast<Explicit&&>(fourth)), "explicit safe move assignment");
    static_assert(noexcept(array_first = array_second), "array copy assignment");
    static_assert(!noexcept(array_first = static_cast<ArrayOwner&&>(array_second)), "array move assignment");
    if (&(first = second) != &first || &(third = fourth) != &third) return 1;
    array_second.values[0].number = 5;
    array_second.values[1].number = 6;
    if (&(array_first = array_second) != &array_first
        || array_first.values[0].number != 5 || array_first.values[1].number != 6) return 2;
    if (&(array_second = static_cast<ArrayOwner&&>(array_first)) != &array_second
        || array_second.values[0].number != 5 || array_second.values[1].number != 6
        || array_first.values[0].number != 0 || array_first.values[1].number != 0) return 3;
    return copies != 4 || moves != 2;
}
