// EXPECT_COMPILE_FAIL: 1
struct Base { int get() const { return 7; } };
struct Derived : private Base {};
int (Derived::*pointer)(void) const = &Base::get;
