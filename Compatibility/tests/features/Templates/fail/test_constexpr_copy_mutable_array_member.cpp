// EXPECT_COMPILE_FAIL: 1
struct Value { mutable int numbers[2]; };
constexpr Value source{{4, 9}};
int main() {
    constexpr Value invalid = source;
}
