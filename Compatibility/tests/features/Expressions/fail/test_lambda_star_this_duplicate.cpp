// EXPECT_COMPILE_FAIL: 1
struct Value { void test() { auto invalid = [this, *this] {}; } };
int main() { Value value; value.test(); }
