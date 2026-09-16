// EXPECT_COMPILE_FAIL: 1
struct Base {};
struct Derived : Base { using Base::missing; };
