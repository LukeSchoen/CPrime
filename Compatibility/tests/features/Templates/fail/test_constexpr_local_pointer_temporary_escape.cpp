// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
constexpr const int *expired() {
    const Value &object = Value{7};
    const int *pointer = &object.number;
    return pointer;
}
static_assert(*expired() == 7);
int main() {}
