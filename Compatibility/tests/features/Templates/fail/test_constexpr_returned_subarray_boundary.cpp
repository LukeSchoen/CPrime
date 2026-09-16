// EXPECT_COMPILE_FAIL: 1
constexpr int values[2][2] = {{1, 2}, {3, 4}};
constexpr const int *row() { return values[0]; }
constexpr int read(const int *pointer) { return pointer[2]; }
static_assert(read(row()) == 3);
int main() {}
