// EXPECT_COMPILE_FAIL: 1
constexpr int values[2][2] = {{1, 2}, {3, 4}};
constexpr const int *row = values[0];
static_assert(row[2] == 3);
int main() {}
