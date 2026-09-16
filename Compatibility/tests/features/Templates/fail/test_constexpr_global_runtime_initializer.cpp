// EXPECT_COMPILE_FAIL: 1
int runtime_value();
constexpr int invalid = runtime_value();
int main() {}
