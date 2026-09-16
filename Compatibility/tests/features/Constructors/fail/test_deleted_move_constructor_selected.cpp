// EXPECT_COMPILE_FAIL: 1
struct Value {
    Value() {}
    Value(const Value&) {}
    Value(Value&&) = delete;
};
int main() { Value source; Value moved(static_cast<Value&&>(source)); }
