// EXPECT_COMPILE_FAIL: 1
const int values[] = {4, 9};
static_assert(values[1] == 9);
int main() {}
