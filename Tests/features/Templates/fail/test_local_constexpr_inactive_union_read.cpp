// EXPECT_COMPILE_FAIL: 1
union Choice { int first; int second; };
struct Outer { Choice member; };
int main() {
    constexpr Outer value{{4}};
    static_assert(value.member.second == 4);
}
