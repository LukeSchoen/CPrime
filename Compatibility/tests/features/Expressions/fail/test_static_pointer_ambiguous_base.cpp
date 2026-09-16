// EXPECT_COMPILE_FAIL: 1
struct Base {};
struct Left : Base {};
struct Right : Base {};
struct Derived : Left, Right {};
Base* convert(Derived* value) { return static_cast<Base*>(value); }
