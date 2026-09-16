// EXPECT_COMPILE_FAIL: 1
struct Value { mutable int numbers[2]; };
int main() {
    constexpr Value value{{4, 9}};
    static_assert(value.numbers[1] == 9);
}
