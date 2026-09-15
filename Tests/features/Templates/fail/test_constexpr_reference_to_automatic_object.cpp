// EXPECT_COMPILE_FAIL: 1
// C++17 [expr.const]/2.11 and /5: the referent lacks static storage duration.
struct Value { int number; };
int main() {
    constexpr Value value{9};
    const Value &reference = value;
    static_assert(reference.number == 9);
    return reference.number != 9;
}
