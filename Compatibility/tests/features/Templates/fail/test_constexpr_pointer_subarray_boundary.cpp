// EXPECT_COMPILE_FAIL: 1
constexpr int values[2][2] = {{1, 2}, {3, 4}};
constexpr int invalid() {
    const int *row = values[0];
    return row[2];
}
static_assert(invalid() == 3);
int main() {}
