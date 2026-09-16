// EXPECT_COMPILE_FAIL: 1
int main() {
    auto selected = []() -> int {
        if constexpr (false) return "invalid conversion";
        return 7;
    };
    return selected();
}
