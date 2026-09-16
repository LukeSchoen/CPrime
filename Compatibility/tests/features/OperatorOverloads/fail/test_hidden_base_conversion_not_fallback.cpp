// EXPECT_COMPILE_FAIL: 1
struct Base { operator int() const { return 1; } };
struct Derived : Base { operator int() { return 2; } };
int main() { const Derived object; int value = object; return value; }
