// EXPECT_COMPILE_FAIL: 1
int main() {
    if constexpr (constexpr int value = 1) { (void)value; }
    return value;
}
