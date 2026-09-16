// EXPECT_COMPILE_FAIL: 1
struct Value { mutable int number; };
constexpr Value value{9};
int main() {
    const Value &reference = value;
    static_assert(reference.number == 9);
}
