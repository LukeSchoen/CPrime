// EXPECT_COMPILE_FAIL: 1
struct Base { virtual ~Base() {} };
struct Derived : Base {};
int main() { const Base base; return dynamic_cast<Derived *>(&base) != 0; }
