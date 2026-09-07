// EXPECT_COMPILE_FAIL: 1
struct Base {};
struct Derived : Base {};
int main() { Base base; return dynamic_cast<Derived *>(&base) != 0; }
