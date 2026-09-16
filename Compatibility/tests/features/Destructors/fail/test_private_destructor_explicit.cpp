// EXPECT_COMPILE_FAIL: 1
struct Value { private: ~Value() {} };
void destroy(Value *value) { value->~Value(); }
int main() {}
