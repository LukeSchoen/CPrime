// EXPECT_COMPILE_FAIL: 1
struct Value { ~Value() = delete; };
void destroy(Value *value) { value->~Value(); }
int main() { return 0; }
