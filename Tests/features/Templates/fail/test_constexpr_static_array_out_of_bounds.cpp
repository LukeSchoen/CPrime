// EXPECT_COMPILE_FAIL: 1
constexpr int first[] = {4, 9};
constexpr int second[] = {7, 8};
static_assert(first[2] == 7);
int main() {}
