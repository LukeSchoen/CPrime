// EXPECT_COMPILE_FAIL: 1
struct Inner { int number; };
struct Outer { mutable Inner member; };
int main() {
    constexpr Outer value{{4}};
    static_assert(value.member.number == 4);
}
