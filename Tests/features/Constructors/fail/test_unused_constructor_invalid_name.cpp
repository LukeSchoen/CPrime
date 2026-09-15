// EXPECT_COMPILE_FAIL: 1
struct Value {
    int value;
    Value() { value = nonexistent_value; }
};
int main() {}
