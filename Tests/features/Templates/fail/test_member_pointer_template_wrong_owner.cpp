// EXPECT_COMPILE_FAIL: 1
struct A { int value; };
struct B { int value; };
template<int A::*Member> struct Tag {};
Tag<&B::value> invalid;
