// EXPECT_COMPILE_FAIL: 1
struct Scalar { operator int &(); operator long &(); };
void f(Scalar &x) { ++x; }
