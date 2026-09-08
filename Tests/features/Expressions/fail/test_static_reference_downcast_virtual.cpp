// EXPECT_COMPILE_FAIL: 1
struct Base {};
struct Derived : virtual Base {};
Derived &convert(Base &base) { return static_cast<Derived&>(base); }
