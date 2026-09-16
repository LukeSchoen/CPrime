// EXPECT_COMPILE_FAIL: 1
struct Number { int value; };
struct Value { mutable Number number; };
struct Outer { Value values[2]; };
int main() {
    constexpr Outer source{{{{4}}, {{9}}}};
    constexpr Outer invalid = source;
}
