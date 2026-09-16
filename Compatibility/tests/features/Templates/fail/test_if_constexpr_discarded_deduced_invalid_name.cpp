// EXPECT_COMPILE_FAIL: 1
auto selected() {
    if constexpr (false) return missing_value;
    return 7;
}
int main() {}
