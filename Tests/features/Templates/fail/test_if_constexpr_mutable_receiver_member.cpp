// EXPECT_COMPILE_FAIL: 1
struct Condition {
    mutable int value;
    constexpr explicit operator bool() const { return value != 0; }
};
constexpr Condition object{3};
int main() { if constexpr (object) return 0; }
