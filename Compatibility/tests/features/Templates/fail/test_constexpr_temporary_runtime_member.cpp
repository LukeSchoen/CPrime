// EXPECT_COMPILE_FAIL: 1
int runtime_value();
struct Value { int number; };
static_assert(Value{runtime_value()}.number == 9);
int main() {}
