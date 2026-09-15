// EXPECT_COMPILE_FAIL: 1
struct Value { int number; };
const Value value{9};
static_assert(value.number == 9);
int main() {}
