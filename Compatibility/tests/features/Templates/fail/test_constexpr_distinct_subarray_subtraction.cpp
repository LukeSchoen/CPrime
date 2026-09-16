// EXPECT_COMPILE_FAIL: 1
constexpr int values[2][2] = {{1, 2}, {3, 4}};
static_assert(values[1] - values[0] == 2);
int main() {}
