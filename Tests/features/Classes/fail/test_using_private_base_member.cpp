// EXPECT_COMPILE_FAIL: 1
class Base { void hidden(); };
struct Derived : Base { using Base::hidden; };
