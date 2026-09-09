// EXPECT_COMPILE_FAIL: 1
struct Value { int read(int value) { return value; } };
int wrap(int value) { return value; }
int main() { Value value; return wrap(value.read)(3); }
