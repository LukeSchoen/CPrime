// EXPECT_COMPILE_FAIL: 1
int runtime_value();
struct Value { int number; };
int main() {
    constexpr Value invalid{runtime_value()};
    return invalid.number;
}
