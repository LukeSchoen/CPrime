// EXPECT_COMPILE_FAIL: 1
struct Value { private: ~Value() {} };
struct Unrelated {
    static void destroy(Value *value) { value->~Value(); }
};
int main() {}
