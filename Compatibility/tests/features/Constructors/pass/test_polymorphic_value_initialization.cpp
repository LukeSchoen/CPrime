// EXPECT_EXIT: 0
struct Value { int number; virtual int read() { return number; } };
int read(Value& value) { return value.read(); }
int main() { Value value = Value(); return value.number != 0 || read(value) != 0; }
