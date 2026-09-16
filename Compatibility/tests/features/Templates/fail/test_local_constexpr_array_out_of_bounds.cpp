// EXPECT_COMPILE_FAIL: 1
struct Value { int numbers[2]; int following; };
int main() {
    constexpr Value value{{4, 9}, 7};
    static_assert(value.numbers[2] == 7);
}
