// EXPECT_COMPILE_FAIL: 1
int main() {
    if constexpr (int value = 1) return value;
}
