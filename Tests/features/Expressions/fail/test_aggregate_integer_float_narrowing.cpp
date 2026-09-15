// EXPECT_COMPILE_FAIL: 1
struct Value { float number; };
int main() { Value value{16777217}; return value.number == 0; }
