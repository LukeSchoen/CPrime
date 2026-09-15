// EXPECT_COMPILE_FAIL: 1
struct Base { int get() const { return 7; } };
struct Derived : private Base {};
using Pointer = int (Derived::*)() const;
Pointer pointer = static_cast<Pointer>(&Base::get);
