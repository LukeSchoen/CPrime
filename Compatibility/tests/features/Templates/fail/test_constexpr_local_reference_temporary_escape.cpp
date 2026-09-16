// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
constexpr const Value &expired() {
    const Value &alias = Value{7};
    return alias;
}
static_assert(expired().number == 7);
int main() {}
