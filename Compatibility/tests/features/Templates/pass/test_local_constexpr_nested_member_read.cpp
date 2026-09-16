struct Inner { int number; };
struct Outer { int prefix; Inner member; };
int main() {
    constexpr Outer first{1, {4}}, second{2, {9}};
    static_assert(first.member.number == 4);
    static_assert(second.member.number == 9);
    constexpr Outer zero{3};
    static_assert(zero.member.number == 0);
    return first.member.number != 4 || second.member.number != 9;
}
