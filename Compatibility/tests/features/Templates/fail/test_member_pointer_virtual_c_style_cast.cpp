// EXPECT_COMPILE_FAIL: 1
struct Base { int get() const { return 7; } };
struct Derived : virtual Base {};
using Pointer = int (Derived::*)() const;
Pointer pointer = (Pointer)&Base::get;
