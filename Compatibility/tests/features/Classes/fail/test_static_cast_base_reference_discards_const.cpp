// EXPECT_COMPILE_FAIL: 1
struct Base { int value; };
struct Derived : Base {};
Base& convert(const Derived& value) { return static_cast<Base&>(value); }
int main() { Derived value; convert(value).value=1; return 0; }
