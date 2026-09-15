// EXPECT_COMPILE_FAIL: 1
struct Value {
    Value() {}
    Value(const Value&) = delete;
};
int main() { Value source; Value copy = source; }
