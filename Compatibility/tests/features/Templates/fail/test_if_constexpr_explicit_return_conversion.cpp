// EXPECT_COMPILE_FAIL: 1
int selected() {
    if constexpr (false) return "invalid conversion";
    return 7;
}
int main() {}
