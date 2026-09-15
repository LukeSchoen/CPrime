// EXPECT_COMPILE_FAIL: 1
int runtime_value();
const int value = runtime_value();
constexpr const int *pointer = &value;
static_assert(*pointer == 9);
int main() {}
