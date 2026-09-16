// EXPECT_COMPILE_FAIL: 1
struct Value { int read(int) = delete; int read(double) { return 0; } };
int (Value::*pointer)(int) = &Value::read;
int main() { return 0; }
