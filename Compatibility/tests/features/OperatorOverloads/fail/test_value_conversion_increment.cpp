// EXPECT_COMPILE_FAIL: 1
struct Scalar { operator int(); };
void f(Scalar &x) { x++; }
