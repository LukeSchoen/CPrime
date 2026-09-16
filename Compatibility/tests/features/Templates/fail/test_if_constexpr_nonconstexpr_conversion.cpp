// EXPECT_COMPILE_FAIL: 1
struct Condition {
    constexpr int unrelated() const { return 1; }
    explicit operator bool() const { return true; }
};
constexpr Condition object{};
int main() { if constexpr (object) return 0; }
