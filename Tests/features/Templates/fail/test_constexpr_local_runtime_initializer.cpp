// EXPECT_COMPILE_FAIL: 1
int runtime_value();
int main() {
    constexpr int invalid = runtime_value();
    return invalid;
}
