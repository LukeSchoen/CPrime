struct Inner { int number; };
struct Value { int prefix; Inner member; };
int main() {
    constexpr Value source{3, {9}};
    constexpr Value copy = source;
    static_assert(copy.prefix == 3);
    static_assert(copy.member.number == 9);
    constexpr Value subobject{7, source.member};
    static_assert(subobject.prefix == 7);
    static_assert(subobject.member.number == 9);
    constexpr Value zero{};
    constexpr Value zero_copy = zero;
    static_assert(zero_copy.prefix == 0);
    static_assert(zero_copy.member.number == 0);
    return copy.member.number != 9;
}
