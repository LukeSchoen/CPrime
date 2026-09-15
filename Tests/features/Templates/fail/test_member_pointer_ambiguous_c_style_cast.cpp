// EXPECT_COMPILE_FAIL: 1
struct Base { int get() const { return 7; } };
struct Left : Base {};
struct Right : Base {};
struct Derived : Left, Right {};
using Pointer = int (Derived::*)() const;
Pointer pointer = (Pointer)&Base::get;
