// EXPECT_COMPILE_FAIL: 1
struct Base { int get() const { return 7; } };
struct Derived : virtual Base {};
int (Derived::*pointer)() const = &Base::get;
