// EXPECT_COMPILE_FAIL: 1
int test(int value) {
    if constexpr (value) return 1;
    return 0;
}
int main() {}
