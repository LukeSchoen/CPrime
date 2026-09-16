// EXPECT_COMPILE_FAIL: 1
struct Value {
    int first = second;
    int second = 9;
};
constexpr Value invalid{};
int main() {}
