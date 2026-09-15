// EXPECT_COMPILE_FAIL: 1
inline int test() {
    if constexpr (false) return missing_value;
    return 0;
}
int main() {}
