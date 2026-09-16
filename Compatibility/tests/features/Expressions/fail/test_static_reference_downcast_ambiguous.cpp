// EXPECT_COMPILE_FAIL: 1
struct Base {};
struct Left : Base {};
struct Right : Base {};
struct Derived : Left, Right {};
Derived &convert(Base &base) { return static_cast<Derived&>(base); }
