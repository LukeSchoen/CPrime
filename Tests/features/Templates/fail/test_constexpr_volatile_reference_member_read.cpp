// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
constexpr Value value{9};
int main() {
    const volatile Value &reference = value;
    static_assert(reference.number == 9);
}
