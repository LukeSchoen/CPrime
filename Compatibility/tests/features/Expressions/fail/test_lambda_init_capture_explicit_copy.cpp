// EXPECT_COMPILE_FAIL: 1
struct Value { Value() {} explicit Value(const Value &) {} };
int main() { Value value; auto closure = [copy = value]() {}; }
